PROJECT(toy-gc-learning CXX)

cmake_minimum_required(VERSION 3.15)

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_CXX_STANDARD 17)

set(BUILD_SHARED_LIBS TRUE)

add_subdirectory(third_party)

add_subdirectory(src)

