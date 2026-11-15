/*
 * CORTEXM_SysTick: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LED2 toggles every 50 ms, timed by the 1 ms SysTick.
 */

#include "board.h"
#include "led.h"

#include <libstefi/cortex.h>
#include <libstefi/systick.h>

void main() {
    board_init();                    // systick_init(80 MHz): reload 80000 - 1 cycles per ms
    led_on(LED2);
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(50);
    }
}
