# NUCLEO-L476RG wired to the USB-oscilloscope front-end. Same MCU family as
# nucleo_l476 (STM32L4G4), but the scope needs the full 128 KB of RAM.
set(STEFI_BOARD_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../configs/link_stm32l476.ld)
