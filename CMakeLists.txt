cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
include(pico-sdk/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(electrolysis_rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(u8g2)

add_executable(${PROJECT_NAME}
    src/main.c
    src/oled.c
    src/probe.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pwm
    hardware_i2c
    hardware_adc
    u8g2
)

# no need to optimise for size as the project is relatively small
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffunction-sections
    -fdata-sections
    -O2
)

target_compile_options(u8g2 PRIVATE
    -ffunction-sections
    -fdata-sections
    -O2
)

target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--gc-sections
    -Wl,--print-gc-sections
)


pico_add_extra_outputs(${PROJECT_NAME})
