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

# Board selection: each board is a subdirectory of boards/ providing
# board.{h,c} and, optionally, board.cmake (sets STEFI_MCU_FAMILY when the
# board is not an STM32L4/G4).
set(STEFI_BOARD "nucleo_l476" CACHE STRING "Target board (subdirectory of boards/)")
set_property(CACHE STEFI_BOARD PROPERTY STRINGS nucleo_l476 nucleo_l476rg rohmi_g431 nucleo_g431kb nucleo_f767zi nucleo_l476_scope)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boards/${STEFI_BOARD}/board.c)
    message(FATAL_ERROR "Unknown STEFI_BOARD '${STEFI_BOARD}' — no boards/${STEFI_BOARD}/board.c")
endif()
message(STATUS "stefi-lib board: ${STEFI_BOARD}")

# MCU family: STM32L4G4 (L476/G431 share one register layout, the original
# target) or STM32F7 (F767, Cortex-M7: different bus map, stream DMA, USB OTG).
set(STEFI_MCU_FAMILY STM32L4G4)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boards/${STEFI_BOARD}/board.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/boards/${STEFI_BOARD}/board.cmake)
endif()
message(STATUS "stefi-lib MCU family: ${STEFI_MCU_FAMILY}")

# Family-independent HAL sources (register maps selected in src/internal/
# by the STEFI_MCU_* define).
set(STEFI_HAL_COMMON_SOURCES
        src/gpio.c
        src/util.c
        src/syscalls.c
        src/systick.c
        src/interrupts.c
        src/timer.c
        src/ringbuffer.c
)

if(STEFI_MCU_FAMILY STREQUAL "STM32F7")
    add_library(libstefi
            ${STEFI_HAL_COMMON_SOURCES}
            src/stm32f7/startup.c
            src/stm32f7/peripheral.c
            src/stm32f7/system.c
            src/stm32f7/uart.c
            src/stm32f7/dma.c
            src/usb_cdc_dwc2.c
    )
elseif(STEFI_MCU_FAMILY STREQUAL "STM32L4G4")
    add_library(libstefi
            ${STEFI_HAL_COMMON_SOURCES}
            src/peripheral.c
            src/uart.c
            src/startup.c
            src/system.c
            src/i2c.c
            src/rng.c
            src/spi.c
            src/adc.c
            src/wwdg.c
            src/flash.c
            src/dma.c
            src/usb_cdc_dwc2.c     # L4 OTG_FS (not on G4)
            src/usart.c
            src/pwr.c
            src/rtc.c
            src/iwdg.c
            src/cordic.c           # G4 only
            src/fmac.c             # G4 only
            src/crc.c
            src/comp.c             # L4
            src/dac.c              # L4
            src/opamp.c            # L4
            src/lptim.c            # L4
            src/i2c_xfer.c
            src/cortex.c
            src/firewall.c         # L4
    )
else()
    message(FATAL_ERROR "Unknown STEFI_MCU_FAMILY '${STEFI_MCU_FAMILY}' (STM32L4G4 or STM32F7)")
endif()

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

# Layer 2 (board) and layer 3 (components). Dependency direction is
# app -> components -> board -> libstefi; the HAL links nothing above it.
add_subdirectory(boards)
add_subdirectory(components/simple_io)
add_subdirectory(components/display_oled)
add_subdirectory(components/motor)
add_subdirectory(components/line_sensor)
add_subdirectory(components/imu)
add_subdirectory(components/edge_trigger)
add_subdirectory(components/ref_dac)
add_subdirectory(components/cmd_line)
add_subdirectory(components/event_fsm)
add_subdirectory(components/dsp)
add_subdirectory(components/parallel_adc)

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
    elseif(STEFI_BOARD STREQUAL "nucleo_l476rg")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/12-NucleoL476RG)
    elseif(STEFI_BOARD STREQUAL "nucleo_g431kb")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/11-NucleoG431KB)
    elseif(STEFI_BOARD STREQUAL "nucleo_f767zi" OR STEFI_BOARD STREQUAL "nucleo_l476_scope")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/10-UsbOscilloscope)
    endif()
endif ()