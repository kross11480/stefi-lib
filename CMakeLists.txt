cmake_minimum_required(VERSION 3.24)

# Default to the bundled cross toolchain when none is given, so a plain
# `cmake -B build` still works. Consuming projects (add_subdirectory /
# FetchContent) set CMAKE_TOOLCHAIN_FILE themselves before their project().
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-arm.cmake
            CACHE FILEPATH "CMake toolchain file")
endif()

# project settings
project(stefi VERSION 0.1.0 LANGUAGES C ASM)
set(CMAKE_C_STANDARD 11)

add_library(libstefi
        src/gpio.c
        src/peripheral.c
        src/util.c
        src/uart.c
        src/startup.c
        src/syscalls.c
        src/system.c
        src/systick.c
        src/interrupts.c
        src/timer.c
        src/i2c.c
        src/rng.c
        src/ringbuffer.c
        src/spi.c
        src/adc.c
        src/wwdg.c
        src/flash.c
        src/dma.c
)

add_library(stefi::hal ALIAS libstefi)

# MCU flags, linker script and runtime link options are attached to the
# libstefi target (not directory-wide), so they propagate to anything that
# links stefi::hal — including executables in external projects.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target-mcu.cmake)

target_include_directories(libstefi PUBLIC include)

# Private implementation headers
target_include_directories(libstefi
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
)

# Board selection: each board is a subdirectory of boards/ providing
# board.{h,c}.
set(STEFI_BOARD "nucleo_l476" CACHE STRING "Target board (subdirectory of boards/)")
set_property(CACHE STEFI_BOARD PROPERTY STRINGS nucleo_l476 rohmi_g431)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boards/${STEFI_BOARD}/board.c)
    message(FATAL_ERROR "Unknown STEFI_BOARD '${STEFI_BOARD}' — no boards/${STEFI_BOARD}/board.c")
endif()
message(STATUS "stefi-lib board: ${STEFI_BOARD}")

# Layer 2 (board) and layer 3 (components). Dependency direction is
# app -> components -> board -> libstefi; the HAL links nothing above it.
add_subdirectory(boards)
add_subdirectory(components/simple_io)
add_subdirectory(components/display_oled)
add_subdirectory(components/motor)
add_subdirectory(components/line_sensor)
add_subdirectory(components/imu)

# Examples and tests are built by default only when stefi-lib is the
# top-level project, never when it is consumed by an application.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(STEFI_BUILD_EXAMPLES "Build the bundled examples and on-hardware tests" ON)
else()
    option(STEFI_BUILD_EXAMPLES "Build the bundled examples and on-hardware tests" OFF)
endif()

if(STEFI_BUILD_EXAMPLES)
    # Automatically add all subdirectories under a given directory
    function(add_subdirectories base_dir)
        file(GLOB subdirectories LIST_DIRECTORIES true ${base_dir}/*)
        foreach(subdir ${subdirectories})
            if(IS_DIRECTORY ${subdir})
                add_subdirectory(${subdir})
            endif()
        endforeach()
    endfunction()
    # Automatically add all example and test projects. Examples 1-7 and the
    # tests are written against the STefi adapter devices (buttons, OLED),
    # nucleo_l476 only; the Rohmi board has its own board test.
    if(STEFI_BOARD STREQUAL "nucleo_l476")
        add_subdirectories(${CMAKE_CURRENT_SOURCE_DIR}/examples)
        add_subdirectories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
    elseif(STEFI_BOARD STREQUAL "rohmi_g431")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/8-Rohmi-BoardTest)
    endif()
endif ()