//
// Created by mwish on 2019/12/29.
//

#include "marksweep/allocator.h"
#include "exceptions.h"

#include <list>
#include <memory>
#include <exception>
#include <cassert>

// The minimal size to allocate from heap.
constexpr int MinHeapAlloc = 1024;

template <typename T>
static void kr_free(allocatorHeader* free_list, allocatorHeader* using_mem, T* free_mem);

class allocatorHeader;

//class allocatorHeaderIter {
//    friend class allcatorHeader;
//
//    allocatorHeader* beg;
//    allocatorHeader* current;
//
//    allocatorHeaderIter(allocatorHeader* cur): beg(cur), current(cur) {}
//};

//std::list<allocatorHeader> free_list;

template <typename T>
static allocatorHeader* more_heap(allocatorHeader* free_list, allocatorHeader* using_mem, size_t num_units) {
    if (num_units < MinHeapAlloc) num_units = MinHeapAlloc;
    char* heap_mem = sbrk(num_units);
    if (heap_mem == nullptr) {
        throw std::bad_alloc();
    }

    auto header = reinterpret_cast<allocatorHeader*>(heap_mem);
    header->size = num_units;

    kr_free(free_list, using_mem, reinterpret_cast<T*>(header + 1));
    return header;
}

template <typename T>
static void kr_free(allocatorHeader* free_list, allocatorHeader* using_mem, T* free_mem) {
    auto* header = reinterpret_cast<allocatorHeader*>(free_mem - 1);

    assert(free_list != nullptr);

    auto free_start = free_list;
    // what if header is the minimal?
    while (!(free_start > header && free_start->next < header) && free_start->next != free_list) {
        free_start = free_start->next;
    }

    header->next = free_start->next;
    free_start->next = header;

    // delete in using_mem
    free_start = using_mem;
    while (true) {
        if (free_start->next == header) {
            free_start->next = header->next;
            break;
        }
    }
}

template <typename T>
static T* kr_alloc(allocatorHeader* free_list, allocatorHeader* using_mem, size_t sz) {
    bool allocated{false};
    NotImplemented();
}


template<typename T>
T *MarkSweepAllocator<T>::allocate(size_t num) {
    return kr_alloc<T>(this->free, this->use_mem, (size_t)num * sizeof(T));
}

template<typename T>
void MarkSweepAllocator<T>::deallocate(T * to_free) {
    return kr_free(this->free, this->use_mem, to_free);
}

//template<typename T>
//MarkSweepAllocator<T>::MarkSweepAllocator() {
//    this->free = &base;
//    this->free->next = &base;
//    this->free->size = 0;
//}