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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(screen C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(pico-displayDrivs)

# Add executable. Default name is the project name, version 0.1

add_executable(screen screen.c FreeMono9pt7b.h FreeMono12pt7b.h FreeMonoBold12pt7b.h FreeMono18pt7b.h FreeMonoBold18pt7b.h FreeMono24pt7b.h FreeMonoBold24pt7b.h rizzi.h dice.h rizzi_color.h ui.c)

pico_set_program_name(screen "screen")
pico_set_program_version(screen "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(screen 0)
pico_enable_stdio_usb(screen 1)

# Add the standard library to the build
target_link_libraries(screen
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(screen PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(screen 
        hardware_spi
        ili9341
        gfx
        )

pico_add_extra_outputs(screen)