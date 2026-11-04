/*
 * LPTIM_PulseCounter: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * LPTIM1 (LSI) counts edges on PB5 in Stop 1; every 1001 edges LED2 toggles.
 */

#include "board.h"
#include "led.h"

#include <libstefi/lptim.h>
#include <libstefi/pwr.h>
#include <libstefi/systick.h>

#include <stddef.h>

static void on_arr_match(void *aux) {
    (void) aux;
    led_toggle(LED2);
}

void main() {
    board_init();
    lptim_init(LPTIM_CLK_LSI, LPTIM_COUNT_IN1_SAMPLED, 0);
    lptim_counter_start(1000, on_arr_match, NULL);
    while(1) pwr_enter_stop1();
}
