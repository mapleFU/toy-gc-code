include_directories(include)

add_subdirectory(marksweep)

add_executable(test-compile test.cpp)

find_package(fmt)
target_link_libraries(test-compile fmt::fmt)