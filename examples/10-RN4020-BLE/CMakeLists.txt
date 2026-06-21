add_executable(rn4020_ble rn4020.c)

# board_init (board) + printf/gpio/systick (HAL, transitive). UART4 is driven
# directly in the test since the HAL UART only covers USART2.
target_link_libraries(rn4020_ble PRIVATE stefi::simpleio stefi::oled)

# UART4 loopback diagnostic (jumper PA0<->PA1) to isolate MCU vs module.
add_executable(uart4_loopback loopback.c)
target_link_libraries(uart4_loopback PRIVATE stefi::simpleio stefi::oled)

# Wiring/power diagnostic (no rewire): senses module TX level on PA1.
add_executable(ble_pincheck pincheck.c)
target_link_libraries(ble_pincheck PRIVATE stefi::simpleio stefi::oled)
