add_executable(stefi_boardtest boardtest.c)

# board_init + LEDs/buttons (simpleio), OLED (oled); SPI/gpio come from the
# HAL transitively. Written against the nucleo_l476 board tables.
target_link_libraries(stefi_boardtest PRIVATE stefi::board stefi::simpleio stefi::oled)
