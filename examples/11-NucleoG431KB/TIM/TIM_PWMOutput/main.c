/*
 * TIM_PWMOutput (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * TIM1 at 170 MHz, prescaler 2 -> 85 MHz counter clock, period 1000 ->
 * 85 kHz PWM on four channels, active high:
 *   CH1 PA8  (AF6)  50 %      CH2 PA9  (AF6)  37.5 %
 *   CH3 PA10 (AF6)  25 %      CH4 PA11 (AF11) 12.5 %
 */
#include "g4_example.h"
#include "libstefi/timer.h"

#define PERIOD 1000u

static const struct {
    gpio_id_t pin;
    afr_t af;
    uint32_t pulse;
} ch[4] = {
    {A8,  AF6,  PERIOD / 2},
    {A9,  AF6,  PERIOD * 375 / 1000},
    {A10, AF6,  PERIOD / 4},
    {A11, AF11, PERIOD * 125 / 1000},
};

void main(void)
{
    board_init();

    timer_init(TIMER1);
    timer_set_period(TIMER1, 2, PERIOD);
    for (uint32_t i = 0; i < 4; i++) {
        timer_channel_t c = (timer_channel_t) (i + 1);
        gpio_set_alternate_function(ch[i].pin, ch[i].af);
        gpio_set_output_speed(ch[i].pin, HIGH_S);
        gpio_set_mode(ch[i].pin, MODER_AF);
        timer_set_mode_pwm(TIMER1, c);
        timer_set_compare(TIMER1, c, ch[i].pulse);
        timer_cc_enable(TIMER1, c, false);
        timer_set_oc_polarity(TIMER1, c, true);
    }
    timer_start(TIMER1);

    for (;;);
}
