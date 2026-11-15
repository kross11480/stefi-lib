/*
 * TIM_PWMOutput: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM1 24 kHz PWM: PA8 50 %, PA9 37.5 %, PA10 25 %, PA11 12.5 %.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

#define PERIOD 666u
static const gpio_id_t pins[4] = {A8, A9, A10, A11};
static const uint32_t pulse[4] = {PERIOD / 2, PERIOD * 375 / 1000, PERIOD / 4, PERIOD * 125 / 1000};

void main() {
    board_init();
    timer_init(TIMER1);
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 16000000u), PERIOD);
    for (int i = 0; i < 4; i++) {
        timer_channel_t c = (timer_channel_t) (i + 1);
        gpio_set_alternate_function(pins[i], AF1);
        gpio_set_mode(pins[i], MODER_AF);
        timer_set_mode_pwm(TIMER1, c);
        timer_set_compare(TIMER1, c, pulse[i]);
        timer_cc_enable(TIMER1, c, false);
        timer_set_oc_polarity(TIMER1, c, true);
    }
    timer_start(TIMER1);
    while(1);
}
