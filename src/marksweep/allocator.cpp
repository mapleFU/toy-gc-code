//
// Created by mwish on 2019/12/29.
//

#include "marksweep/allocator.h"

#if __APPLE__
#include <mach-o/getsect.h>
#endif

#include <cassert>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <unistd.h>

// The minimal size to allocate from heap.
constexpr int MinHeapAlloc = 1024 * 4;

struct allocatorHeader;

struct allocatorHeader {
    // unsigned long's length is equal to word's length.
    std::size_t size;
    // like k&r list
    allocatorHeader *next;
};

// the starter pointer of freelist
static allocatorHeader *free_list = nullptr;
// used pointer, it start means the heap range begin, it's end means the end of
// used. Points to first used block of memory.
static allocatorHeader *usedp = nullptr;
// base list
static allocatorHeader base;

// std::list<allocatorHeader> free_list;

static allocatorHeader *more_heap(size_t num_units) {
    if (num_units < MinHeapAlloc)
        num_units = MinHeapAlloc;
    char *heap_mem = reinterpret_cast<char *>(sbrk(num_units));
    if (heap_mem == nullptr) {
        throw std::bad_alloc();
    }

    auto header = reinterpret_cast<allocatorHeader *>(heap_mem);
    header->size = num_units;

    gc_free(header + 1);
    return header;
}

// `gc_free` may use to free, but now it may be used to "add back to linked
// list". panic: if `free_list` is nullptr, this function will panic.
void gc_free(void *free_mem) {
    auto *header = reinterpret_cast<allocatorHeader *>(free_mem) - 1;

    assert(free_list != nullptr);

    auto free_ptr = free_list;
    while (!(free_ptr < header && free_ptr->next > header)) {
        // corner case: | | header
        // header is on the top
        if ((free_ptr->next < free_ptr || free_ptr->next == free_ptr) &&
            header > free_ptr) {
            break;
        }
        free_ptr = free_ptr->next;
    }

    // now, the case may be:
    // free |  header | free->next
    // we need to merge linked list

    // merge one: header and free->next
    if (header + header->size == free_ptr->next) {
        header->size += free_ptr->next->size;
        header->next = free_ptr->next->next;
    } else {
        header->next = free_ptr->next;
    }

    // merge two:
    if (free_ptr->size + free_ptr == header) {
        free_ptr->size += header->size;
        free_ptr->next = header->next;
    } else {
        free_ptr->next = header;
    }
}

void *gc_alloc(size_t nbytes) {
    allocatorHeader *p, *prevp;
    std::size_t nunits;
    // force padding to num of units
    nunits =
        (nbytes + sizeof(allocatorHeader) - 1) / sizeof(allocatorHeader) + 1;
    if ((prevp = free_list) == nullptr) {
        base.next = prevp = free_list = &base;
        base.size = 0;
    }

    // using first fir algorithm.
    // fetch a enough size from
    for (p = prevp->next;; prevp = p, p = p->next) {
        if (p->size >= nunits) { /* big enough */
            if (p->size == nunits) /* exactly */
                // just remove and return p.
                prevp->next = p->next;
            else { /* allocate tail end */
                // Note: The pointer is not need to change
                // because the final p we return is the tail part
                // |  | --> |  | return |
                p->size -= nunits;
                p += p->size;
                p->size = nunits;
            }
            free_list = prevp;
            // note: the code here updates the usedp
            /* Add to p to the used list. */
            if (usedp == nullptr)
                usedp = p->next = p;
            else {
                p->next = usedp->next;
                usedp->next = p;
            }

            return (void *)(p + 1);
        }
        // p == freep, means back to the start but doesn't got enough space.
        if (p == free_list) { /* wrapped around free list */
            if ((p = more_heap(nunits)) == nullptr) {
                return nullptr; /* none left */
            }
            // TODO: this maybe bad
            prevp = p; p = p->next;
        }
    }
}

// c means 12 = 2^3 + 2^2, that is 0000 0110
inline static allocatorHeader *untag(const allocatorHeader *p) {
    return reinterpret_cast<allocatorHeader *>(
        ((reinterpret_cast<std::size_t>(p)) & 0xfffffffc));
}

/*
 * Scan a region of memory and mark any items in the used list appropriately.
 * Both arguments should be word aligned.
 */
static void scan_region(std::size_t *sp, std::size_t *end) {
    allocatorHeader *bp;

    for (; sp < end; sp++) {
        std::size_t v = *sp;
        bp = usedp;
        do {
            // Note: this part do a checking: does sp point to a field in bp |s
            // e| --> [s, e)
            if (reinterpret_cast<size_t>(bp + 1) <= v &&
                reinterpret_cast<size_t>(bp + 1 + bp->size) > v) {
                // tag
                bp->next = reinterpret_cast<allocatorHeader *>(
                    (reinterpret_cast<std::size_t>(bp->next) | 1));
                break;
            }
        } while ((bp = untag(bp->next)) != usedp);
    }
}

/*
 * Scan the marked blocks for references to other unmarked blocks.
 */
static void scan_heap() {
    std::size_t *vp;
    allocatorHeader *bp, *up;

    for (bp = untag(usedp->next); bp != usedp; bp = untag(bp->next)) {
        if (!((std::size_t)bp->next & 1))
            continue;
        // just like the code above, checks field in bp's allocated memory.
        for (vp = (std::size_t *)(bp + 1);
             vp < reinterpret_cast<std::size_t *>((bp + bp->size + 1)); vp++) {
            std::size_t v = *vp;
            up = untag(reinterpret_cast<const allocatorHeader *>(bp->next));
            do {
                if (up != bp && reinterpret_cast<std::size_t>(up + 1) <= v &&
                    reinterpret_cast<std::size_t>(up + 1 + up->size) > v) {
                    up->next = reinterpret_cast<allocatorHeader *>(
                        ((std::size_t)up->next) | 1);
                    break;
                }
            } while ((up = untag(reinterpret_cast<const allocatorHeader *>(
                          up->next))) != bp);
        }
    }
}

static std::size_t stack_bottom;

/*
 * Find the absolute bottom of the stack and set stuff up.
 */
static void GC_init() {
    static bool initted(false);
    FILE *statfp;

    if (initted)
        return;

    initted = true;

    // Note: "/proc/self" code may not work on MacOS, so we need pid.
    // https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
    // Mac should use vmmap, currently it's unimplemented.
    //    auto s = fmt::format("/proc/{}/stat", getpid());
    statfp = fopen("/proc/self/stat", "r");
    assert(statfp != nullptr);
    fscanf(statfp,
           "%*d %*s %*c %*d %*d %*d %*d %*d %*u "
           "%*lu %*lu %*lu %*lu %*lu %*lu %*ld %*ld "
           "%*ld %*ld %*ld %*ld %*llu %*lu %*ld "
           "%*lu %*lu %*lu %lu",
           &stack_bottom);
    fclose(statfp);

    base.next = free_list = &base;
    base.size = 0;
}

std::once_flag gc_init;

void global_gc_init() {
    // TODO: call_once may cause bug, please fix it latter.
    //    std::call_once(gc_init, GC_init);
    GC_init();
}

// the address of etext is the last address past the text segment.
// The initialized data segment immediately follows the text segment and
// thus the address of etext is the start of the initialized data segment.
// end: the address of end is the start of the heap, or the last address
// past the end of the BSS.
#if __APPLE__
// Note: apple needs
// https://stackoverflow.com/questions/1765969/where-are-the-symbols-etext-edata-and-end-defined
static char end = get_end();
static char etext = get_etext();
#else
extern char end, etext; /* Provided by the linker. */
#endif

/*
 * Mark blocks of memory in use and free the ones not in use.
 */
void GC_collect() {
    allocatorHeader *prevp, *tp;
    allocatorHeader *p;
    std::size_t stack_top;

    if (usedp == nullptr)
        return;

    /* Scan the BSS and initialized data segments. */
    scan_region(reinterpret_cast<size_t *>(&etext),
                reinterpret_cast<size_t *>(&end));

    /* Scan the stack. */
    // https://stackoverflow.com/questions/10461798/asm-code-containing-0-what-does-that-mean
    asm volatile("movl %%ebp, %0" : "=rm"(stack_top));
    scan_region(reinterpret_cast<size_t *>(&stack_top), &stack_bottom);

    /* Mark from the heap. */
    scan_heap();

    /* And now we collect! */
    for (prevp = usedp, p = untag(usedp->next);;
         prevp = p, p = untag(p->next)) {
    next_chunk:
        if (!((std::size_t)p->next & 1)) {
            /*
             * The chunk hasn't been marked. Thus, it must be set free.
             */
            tp = p;
            p = untag(p->next);
            gc_free(tp);

            if (usedp == tp) {
                usedp = nullptr;
                break;
            }

            prevp->next = reinterpret_cast<allocatorHeader *>(
                (std::size_t)p | ((std::size_t)prevp->next & 1));
            goto next_chunk;
        }
        p->next =
            reinterpret_cast<allocatorHeader *>(((std::size_t)p->next) & ~1);
        if (p == usedp)
            break;
    }
}