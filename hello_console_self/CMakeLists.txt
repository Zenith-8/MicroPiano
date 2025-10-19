# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hello_console_self C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hello_console_self
    hello_console_self.c
    hw_config.c
)

pico_set_program_name(hello_console_self "hello_console_self")
pico_set_program_version(hello_console_self "0.1")

# USB stdio on, UART stdio off (we use hardware UART explicitly)
pico_enable_stdio_uart(hello_console_self 0)
pico_enable_stdio_usb(hello_console_self 1)

target_link_libraries(hello_console_self
    pico_stdlib
    hardware_i2c
    hardware_spi
)

# Optionally link FatFs SPI library if present
set(FATFS_SPI_DIR ${CMAKE_CURRENT_LIST_DIR}/../no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI)
if (EXISTS ${FATFS_SPI_DIR}/CMakeLists.txt)
    add_subdirectory(${FATFS_SPI_DIR} build_fatfs_spi)
    target_link_libraries(hello_console_self FatFs_SPI)
    target_include_directories(hello_console_self PRIVATE ${FATFS_SPI_DIR})
endif()

target_include_directories(hello_console_self PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(hello_console_self)