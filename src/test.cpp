//
// Created by mwish on 2019/12/29.
//

#include "marksweep/allocator.h"
#include "exceptions.h"

void gc_call() {
    gc_alloc(2000);
    gc_alloc(2000);
}

int main() {
    global_gc_init();
    gc_call();
    gc_call();
    GC_collect();
}
