cmake_minimum_required(VERSION 3.15)

# test executable for uartium
add_executable(test_uartium
    test_uartium.c
)

target_include_directories(test_uartium PRIVATE "${CMAKE_SOURCE_DIR}")

target_link_libraries(test_uartium PRIVATE uartium)

add_test(NAME uartium_test COMMAND test_uartium)

# Stress test (may run longer)
add_executable(stress_uartium
    stress_uartium.c
)

target_include_directories(stress_uartium PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(stress_uartium PRIVATE uartium)

add_test(NAME uartium_stress COMMAND stress_uartium)
