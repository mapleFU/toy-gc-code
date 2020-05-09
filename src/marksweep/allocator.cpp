//
// Created by mwish on 2019/12/29.
//

#include "marksweep/allocator.h"
#include "exceptions.h"

#include <exception>
#include <memory>
#include <unistd.h>

// The minimal size to allocate from heap.
constexpr int MinHeapAlloc = 1024 * 4;

class allocatorHeader;

struct allocatorHeader {
    // unsigned long's length is equal to word's length.
    std::size_t size;
    // like k&r list
    allocatorHeader *next;
};

// the starter pointer of freelist
static allocatorHeader *free_list = nullptr;
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
        if (free_ptr->next < free_ptr && header > free_ptr) {
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
            return (void *)(p + 1);
        }
        // p == freep, means back to the start but doesn't got enough space.
        if (p == free_list) { /* wrapped around free list */
            if ((p = more_heap(nunits)) == nullptr) {
                return nullptr; /* none left */
            }
        }
    }
}
