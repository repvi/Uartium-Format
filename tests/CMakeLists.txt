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

# visual/print test
add_executable(visual_uartium
    visual_uartium.c
)

target_include_directories(visual_uartium PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(visual_uartium PRIVATE uartium)

add_test(NAME uartium_visual COMMAND visual_uartium)

# visual message test: buffer_message + log_message
add_executable(visual_message
    visual_message.c
)

target_include_directories(visual_message PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(visual_message PRIVATE uartium)

add_test(NAME uartium_visual_message COMMAND visual_message)

add_executable(visual_messages_all
    visual_messages_all.c
)

target_include_directories(visual_messages_all PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(visual_messages_all PRIVATE uartium)

add_test(NAME uartium_visual_all COMMAND visual_messages_all)
