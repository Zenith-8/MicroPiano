# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pio_uart_rx C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pio_uart_rx)

pico_generate_pio_header(pio_uart_rx ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

target_sources(pio_uart_rx PRIVATE uart_rx.c)

target_link_libraries(pio_uart_rx PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        )

pico_add_extra_outputs(pio_uart_rx)

# add url via pico_set_program_url

# Similar to above but uses an interrupt for RX
add_executable(pio_uart_rx_intr)
pico_generate_pio_header(pio_uart_rx_intr ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)
target_sources(pio_uart_rx_intr PRIVATE uart_rx_intr.c)
target_link_libraries(pio_uart_rx_intr PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        pico_async_context_threadsafe_background
        )
pico_add_extra_outputs(pio_uart_rx_intr)
