PROJECT(mark-sweep)

add_library(marksweep INTERFACE)
target_sources(marksweep INTERFACE allocator.h)


