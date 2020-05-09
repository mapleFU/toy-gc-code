//
// Created by mwish on 2019/12/29.
//

#ifndef TOY_GC_LEARNING_ALLOCATOR_H
#define TOY_GC_LEARNING_ALLOCATOR_H

#include <memory>
#include <unistd.h>

extern void *gc_alloc(size_t nbytes);
extern void gc_free(void *free_mem);

template <typename T>
class MarkSweepAllocator {
public:
    T* allocate(size_t num) {
        return gc_alloc(num * sizeof(T));
    }
    // 我就不回收 (((o(*ﾟ▽ﾟ*)o)))
    void deallocate(T*) {}
};


#endif //TOY_GC_LEARNING_ALLOCATOR_H
