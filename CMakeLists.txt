cmake_minimum_required(VERSION 3.21)

project(SparserCpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic)

find_package(gtest CONFIG REQUIRED)
find_package(rapidjson CONFIG REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
