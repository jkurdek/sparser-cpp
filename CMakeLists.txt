cmake_minimum_required(VERSION 3.21)

project(SparserCpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION -std=gnu++2b)

add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic)
# find_program(CLANG_TIDY "clang-tidy")
# if(CLANG_TIDY)
#     set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
# endif()
find_package(GTest CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
