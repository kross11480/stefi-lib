/*
 * TIM_OnePulse: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * A rising edge on PA9 makes TIM1 output one pulse on PA8 (1 ms delay, 3 ms long).
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

void main() {
    board_init();
    gpio_set_alternate_function(A8, AF1);
    gpio_set_alternate_function(A9, AF1);
    gpio_set_pupd(A8, PULL_DOWN);
    gpio_set_pupd(A9, PULL_DOWN);
    gpio_set_mode(A8, MODER_AF);
    gpio_set_mode(A9, MODER_AF);

    timer_init(TIMER1);
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 16000000u), 65536);
    timer_set_mode_opm(TIMER1);
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL2);       // resets PSC/ARR: set them again
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 16000000u), 65536);
    timer_set_slave_mode(TIMER1, TIMER_SLAVE_TRIGGER, TIMER_TS_TI2FP2);
    timer_set_oc_mode(TIMER1, TIMER_CHANNEL1, TIMER_OC_PWM2);
    timer_set_compare(TIMER1, TIMER_CHANNEL1, 16383);
    timer_cc_enable(TIMER1, TIMER_CHANNEL1, false);
    timer_set_oc_polarity(TIMER1, TIMER_CHANNEL1, true);
    timer_cc_enable(TIMER1, TIMER_CHANNEL2, true);
    timer_cc_interrupt(TIMER1, TIMER_CHANNEL2, false);
    while(1);                                        // the trigger starts the counter
}
