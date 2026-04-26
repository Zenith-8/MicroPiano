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
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)



project(controller_module C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(pico-displayDrivs)

# FatFs + SD over SPI (no-OS-FatFS-SD-SPI-RPi-Pico)
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI fatfs_spi_build)

# Add executable. Default name is the project name, version 0.1

add_executable(controller_module
        hw_config.c
        controller_module.cpp
        screen.cpp
        screen.h
        audio_i2s.pio
        pico-mcp2515/include/mcp2515/mcp2515.cpp
        include/FreeSans24pt7b.h
        include/rizzi.h
        include/dice.h
        include/rizzi_color.h
        include/wav_sample.h
        )

pico_set_program_name(controller_module "controller_module")
pico_set_program_version(controller_module "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(controller_module 1)
pico_enable_stdio_usb(controller_module 0)

# Add the standard library to the build
target_link_libraries(controller_module
        pico_stdlib
        pico_multicore
        )

# Add any user requested libraries
target_link_libraries(controller_module
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_clocks
        ili9341
        gfx
        FatFs_SPI
        )

# Add the standard include files to the build
target_include_directories(controller_module PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/pico-mcp2515/include
)

pico_generate_pio_header(controller_module ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

pico_add_extra_outputs(controller_module)
