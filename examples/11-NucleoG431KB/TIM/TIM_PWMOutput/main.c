/*
 * TIM_PWMOutput: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * TIM1 85 kHz PWM: PA8 50 %, PA9 37.5 %, PA10 25 %, PA11 12.5 %.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

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

void main() {
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

    while(1);
}
