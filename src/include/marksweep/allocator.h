//
// Created by mwish on 2019/12/29.
//

#ifndef TOY_GC_LEARNING_ALLOCATOR_H
#define TOY_GC_LEARNING_ALLOCATOR_H

#include <memory>
#include <unistd.h>

// an incomplete data type
struct allocatorHeader;

template <typename T>
class MarkSweepAllocator {
public:
    MarkSweepAllocator();
    T* allocate(size_t num);
    void deallocate(T*);
private:
    allocatorHeader* free;
    allocatorHeader* use_mem = nullptr;

    // Note: It's on the stack.
    allocatorHeader base;
};


#endif //TOY_GC_LEARNING_ALLOCATOR_H
