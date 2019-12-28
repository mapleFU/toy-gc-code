PROJECT(mark-sweep)



add_library(marksweep INTERFACE)
target_sources(marksweep INTERFACE allocator.h)

add_executable(m test.cpp)


