include_directories(include)

add_subdirectory(marksweep)

add_executable(test-compile test.cpp marksweep/allocator.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall --verbose")

find_package(fmt)

target_link_libraries(test-compile fmt::fmt)