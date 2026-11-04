/*
 * TIM_TimeBase: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM3 update every second toggles LED2.
 */

#include "board.h"
#include "led.h"
#include "hardware_timer.h"

void blink_cb() {
    led_toggle(LED2);
}

void main() {
    board_init();
    htimer_init_periodic_ms(TIMER3, 1000, blink_cb);
    htimer_start(TIMER3);
    while(1);
}
