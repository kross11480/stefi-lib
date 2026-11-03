/*
 * TIM_CascadeSynchro (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 *   TIM2 master: 170 MHz / 256 = 664 kHz PWM, 25 % (CH1 PA5, AF1);
 *                TRGO = update event.
 *   TIM3 slave of TIM2 (ITR1), gated mode: it counts one tick per TIM2
 *                update -> 664 kHz / 4 = 166 kHz PWM, 25 % (CH1 PA6, AF2);
 *                TRGO = its update event.
 *   TIM4 slave of TIM3 (ITR2), gated: 166 kHz / 4 = 41.5 kHz PWM, 25 %
 *                (CH1 PA11, AF10).
 * Look at the three waveforms with an oscilloscope.
 */
#include "g4_example.h"
#include "libstefi/timer.h"

static void pwm_ch1(tim_id_t tim, gpio_id_t pin, afr_t af, uint32_t period, uint32_t pulse)
{
    timer_init(tim);
    timer_set_period(tim, 1, period);
    gpio_set_alternate_function(pin, af);
    gpio_set_output_speed(pin, HIGH_S);
    gpio_set_mode(pin, MODER_AF);
    timer_set_mode_pwm(tim, TIMER_CHANNEL1);
    timer_set_compare(tim, TIMER_CHANNEL1, pulse);
    timer_cc_enable(tim, TIMER_CHANNEL1, false);
    timer_set_oc_polarity(tim, TIMER_CHANNEL1, true);
}

void main(void)
{
    board_init();

    pwm_ch1(TIMER2, A5, AF1, 256, 64);
    timer_set_master_mode(TIMER2, TIMER_TRGO_UPDATE, true);

    pwm_ch1(TIMER3, A6, AF2, 4, 1);
    timer_set_slave_mode(TIMER3, TIMER_SLAVE_GATED, TIMER_TS_ITR1);   // ITR1 = TIM2
    timer_set_master_mode(TIMER3, TIMER_TRGO_UPDATE, true);

    pwm_ch1(TIMER4, A11, AF10, 4, 1);
    timer_set_slave_mode(TIMER4, TIMER_SLAVE_GATED, TIMER_TS_ITR2);   // ITR2 = TIM3

    // slaves first, the master last: it gates the chain
    timer_start(TIMER4);
    timer_start(TIMER3);
    timer_start(TIMER2);

    for (;;);
}
