/* TIM_OnePulse (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM1 at 16 MHz counter clock, one-pulse mode: a rising edge on CH2
 * (PA9, input, trigger TI2FP2) starts the counter; CH1 (PA8) in PWM mode 2
 * outputs one pulse: delay 16383 / 16 MHz = 1.02 ms, length
 * (65535 - 16383) / 16 MHz = 3.07 ms. */
#include "l4_example.h"
#include "libstefi/timer.h"

void main(void)
{
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
    for (;;);                                        // the trigger starts the counter
}
