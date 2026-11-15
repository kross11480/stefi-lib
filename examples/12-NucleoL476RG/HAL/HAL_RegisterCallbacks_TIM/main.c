/*
 * HAL_RegisterCallbacks_TIM: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * A callback registered for the TIM3 1 s period interrupt toggles LED2
 * (htimer_init_periodic_ms() registers it, as HAL_TIM_RegisterCallback does).
 */

#include "board.h"
#include "led.h"
#include "hardware_timer.h"

void my_period_elapsed_cb() {
    led_toggle(LED2);
}

void main() {
    board_init();
    htimer_init_periodic_ms(TIMER3, 1000, my_period_elapsed_cb);
    htimer_start(TIMER3);
    while(1);
}
