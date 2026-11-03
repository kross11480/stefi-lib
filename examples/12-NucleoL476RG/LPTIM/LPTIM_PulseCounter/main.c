/* LPTIM_PulseCounter (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * LPTIM1, kernel clock LSI (32 kHz), counts rising edges on IN1 (PB5)
 * while the MCU is in Stop 1. Every 1001 edges (ARR = 1000) the ARR-match
 * interrupt wakes the MCU and toggles LD2. Input edges must not be faster
 * than the LSI samples them (< 32 kHz). */
#include "l4_example.h"
#include "libstefi/lptim.h"
#include "libstefi/pwr.h"

static void on_arr_match(void *aux)
{
    (void) aux;
    led_toggle(LED2);
}

void main(void)
{
    board_init();
    lptim_init(LPTIM_CLK_LSI, LPTIM_COUNT_IN1_SAMPLED, 0);
    lptim_counter_start(1000, on_arr_match, NULL);
    for (;;) pwr_enter_stop1();
}
