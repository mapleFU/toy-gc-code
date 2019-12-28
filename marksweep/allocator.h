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
    MarkSweepAllocator();
public:
    T* allocate(size_t num);
    void deallocate(T*, size_t num);
private:

};


#endif //TOY_GC_LEARNING_ALLOCATOR_H
