cmake_minimum_required(VERSION 3.15)

project(uartium_format LANGUAGES C)

add_library(uartium STATIC
    uartium.c
)

target_include_directories(uartium 
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

enable_testing()

add_subdirectory(tests)