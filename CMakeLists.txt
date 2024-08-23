cmake_minimum_required(VERSION 3.21)

project(SparserCpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic)

if(ENABLE_CLANG_TIDY)
    if(CLANG_TIDY)
    find_program(CLANG_TIDY "clang-tidy")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
    endif()
endif()

find_package(GTest CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
