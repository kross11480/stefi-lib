/*
 * GPIO_IOToggle: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * PA5 (LED2) toggles every 100 ms. Core at 80 MHz.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/systick.h>

void main() {
    board_init();
    while(1) {
        gpio_toggle(A5);
        systick_delay_ms(100);
    }
}
