# MCU build settings, attached to the libstefi target so they travel with it.
# Anything that links stefi::hal — in-tree examples or executables in a
# consuming application project — inherits the correct compile and link flags.
# Included from the top-level CMakeLists.txt after add_library(libstefi), with
# STEFI_MCU_FAMILY already set (STM32L4G4 or STM32F7).

if(STEFI_MCU_FAMILY STREQUAL "STM32F7")
    set(STEFI_CPU_FLAGS -mcpu=cortex-m7 -mthumb)
    set(STEFI_DEFAULT_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../configs/link_stm32f767.ld)
    # PUBLIC: public headers (gpio.h, interrupts.h) pick the register map /
    # IRQ numbers from it, so every consumer must see the same value.
    target_compile_definitions(libstefi PUBLIC STEFI_MCU_STM32F7)
else()
    set(STEFI_CPU_FLAGS -mcpu=cortex-m4 -mthumb)
    set(STEFI_DEFAULT_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../configs/link.ld)
    target_compile_definitions(libstefi PUBLIC STEFI_MCU_STM32L4G4)
endif()

# compiler options (PUBLIC: ABI-relevant, needed by all code linked with the HAL)
target_compile_options(libstefi PUBLIC
        ${STEFI_CPU_FLAGS}
        -ffunction-sections -fdata-sections
        -g -Wall -std=c99 -ffreestanding
)

# A board may bring its own memory layout (boards/<b>/board.cmake).
if(STEFI_BOARD_LINKER_SCRIPT)
    set(STEFI_DEFAULT_LINKER_SCRIPT ${STEFI_BOARD_LINKER_SCRIPT})
endif()

# Linker script: overridable by an application with a different memory
# layout. Empty means "the board's or family default" (configs/link.ld for
# L4/G4, configs/link_stm32f767.ld for the F7, link_stm32l476.ld for
# nucleo_l476_scope).
set(STEFI_LINKER_SCRIPT "" CACHE FILEPATH
        "Linker script for executables linking stefi::hal (empty: MCU family default)")
if(STEFI_LINKER_SCRIPT)
    set(STEFI_LINKER_SCRIPT_EFFECTIVE ${STEFI_LINKER_SCRIPT})
else()
    set(STEFI_LINKER_SCRIPT_EFFECTIVE ${STEFI_DEFAULT_LINKER_SCRIPT})
endif()

# Optional float support in printf, adds ~12K to code size.
option(STEFI_PRINTF_FLOAT "Enable %f support in printf (~12K code size)" ON)

# linker options (INTERFACE: apply when an executable links stefi::hal)
target_link_options(libstefi INTERFACE
        ${STEFI_CPU_FLAGS}
        -nostartfiles #use my startup file to init stack, mem, ...
        --specs=nano.specs #use newlib-nano, a lightweight standard c lib
        -lc -lgcc #links newlib nano, use dummy sys calls
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -T${STEFI_LINKER_SCRIPT_EFFECTIVE}
)

if(STEFI_PRINTF_FLOAT)
    target_link_options(libstefi INTERFACE -u _printf_float)
endif()

# Relink executables when the linker script changes.
set_property(TARGET libstefi APPEND PROPERTY INTERFACE_LINK_DEPENDS ${STEFI_LINKER_SCRIPT_EFFECTIVE})
