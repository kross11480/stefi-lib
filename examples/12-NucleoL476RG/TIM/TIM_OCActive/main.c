/*
 * TIM_OCActive: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM1 CH1..4 (PA8..PA11) go high 1 s, 500, 250, 125 ms after LED2 turns on.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

static const gpio_id_t pins[4] = {A8, A9, A10, A11};
static const uint32_t pulse[4] = {10000, 5000, 2500, 1250};

void main() {
    board_init();
    led_off(LED2);
    timer_init(TIMER1);
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 10000u), 65536);
    for (int i = 0; i < 4; i++) {
        timer_channel_t c = (timer_channel_t) (i + 1);
        gpio_set_alternate_function(pins[i], AF1);
        gpio_set_mode(pins[i], MODER_AF);
        timer_set_oc_mode(TIMER1, c, TIMER_OC_ACTIVE);
        timer_set_compare(TIMER1, c, pulse[i]);
        timer_cc_enable(TIMER1, c, false);
        timer_set_oc_polarity(TIMER1, c, true);
    }
    led_on(LED2);
    timer_start(TIMER1);
    while(1);
}
