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

if (NOT TARGET pico_audio_32b_headers)
    add_library(pico_audio_32b_headers INTERFACE)
    target_include_directories(pico_audio_32b_headers INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
    )
    target_link_libraries(pico_audio_32b_headers INTERFACE
        pico_util_buffer
    )
endif()

if (NOT TARGET pico_audio_32b)
    add_library(pico_audio_32b INTERFACE)

    target_sources(pico_audio_32b INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/audio.cpp
            $<$<NOT:$<BOOL:${PICO_NO_HARDWARE}>>:${CMAKE_CURRENT_LIST_DIR}/audio_utils.S>
    )

    target_link_libraries(pico_audio_32b INTERFACE
        pico_audio_32b_headers
        pico_sync
        pico_util_buffer
    )

    target_include_directories(pico_audio_32b INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
    )

endif()
