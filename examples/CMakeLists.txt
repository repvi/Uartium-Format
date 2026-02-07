cmake_minimum_required(VERSION 3.15)

project(uartium_examples LANGUAGES C)

# Build the example as a standalone project. Include the library/source
# from the parent directory explicitly so this can be configured from
# `examples/` (run `cmake -S . -B build -G Ninja`).

add_executable(uartium-format
	main.c
	"${CMAKE_CURRENT_SOURCE_DIR}/../uartium.c"
)

target_include_directories(uartium-format PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")