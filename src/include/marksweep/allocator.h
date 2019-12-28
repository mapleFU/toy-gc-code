//
// Created by mwish on 2019/12/29.
//

#ifndef TOY_GC_LEARNING_ALLOCATOR_H
#define TOY_GC_LEARNING_ALLOCATOR_H

#include <memory>
#include <unistd.h>

struct allocatorHeader {
    unsigned int size;
    allocatorHeader* next;
};

template <typename T>
class MarkSweepAllocator {
public:
    // TODO: make clear why this should not be in another file.
    MarkSweepAllocator() {
        this->free = &base;
        this->free->next = &base;
        this->free->size = 0;
    }
    T* allocate(size_t num);
    void deallocate(T*);
private:
    allocatorHeader* free;
    allocatorHeader* use_mem = nullptr;

    // Note: It's on the stack.
    allocatorHeader base;
};


#endif //TOY_GC_LEARNING_ALLOCATOR_H
