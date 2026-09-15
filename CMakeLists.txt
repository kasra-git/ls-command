cmake_minimum_required(VERSION 3.16)
project(ls C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_definitions(_POSIX_C_SOURCE=200809L)
add_compile_options(-Wall -Wextra -O2)

add_executable(ls main.c src/src.c)