include_directories(include)

add_subdirectory(marksweep)

add_executable(test-compile test.cpp marksweep/allocator.cpp)

find_package(fmt)
target_link_libraries(test-compile fmt::fmt)