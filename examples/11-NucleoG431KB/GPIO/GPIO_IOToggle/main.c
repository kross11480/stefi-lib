/*
 * GPIO_IOToggle: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * PB8 (LED2) toggles every 100 ms. Core at 170 MHz.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/systick.h>

void main() {
    board_init();                 // 170 MHz, LD2 on PB8 as push-pull output

    while(1) {
        gpio_toggle(B8);
        systick_delay_ms(100);
    }
}
