//
// Created by mwish on 2019/12/29.
//

#ifndef TOY_GC_LEARNING_ALLOCATOR_H
#define TOY_GC_LEARNING_ALLOCATOR_H

#include <memory>
#include <unistd.h>

// an incomplete data type
struct allocatorHeader;

extern void gc_free(void *free_mem);
extern void *gc_alloc(size_t sz);

template <typename T> class MarkSweepAllocator {
  public:
    T *allocate(size_t num) { return gc_alloc(num * sizeof(T)); }
    void deallocate(T *ptr) { gc_free(ptr); }
};

#endif // TOY_GC_LEARNING_ALLOCATOR_H
