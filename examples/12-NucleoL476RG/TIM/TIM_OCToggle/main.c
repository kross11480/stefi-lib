/*
 * TIM_OCToggle: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM1 CH1..4 (PA8..PA11) toggle at 195, 391, 781, 1563 Hz (output compare toggle).
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

static const gpio_id_t pins[4] = {A8, A9, A10, A11};
static const uint32_t step[4] = {40961, 20480, 10240, 5120};

static void advance(timer_channel_t c) {
    uint32_t v = timer_get_compare(TIMER1, c) + step[c - 1];
    timer_set_compare(TIMER1, c, v & 0xFFFFu);
}
static void cc1(void *a) { (void) a; advance(TIMER_CHANNEL1); }
static void cc2(void *a) { (void) a; advance(TIMER_CHANNEL2); }
static void cc3(void *a) { (void) a; advance(TIMER_CHANNEL3); }
static void cc4(void *a) { (void) a; advance(TIMER_CHANNEL4); }

void main() {
    board_init();
    timer_init(TIMER1);
    timer_set_period(TIMER1, (uint16_t) (system_get_core_clock() / 16000000u), 65536);
    const callbackfn_t cb[4] = {cc1, cc2, cc3, cc4};
    for (int i = 0; i < 4; i++) {
        timer_channel_t c = (timer_channel_t) (i + 1);
        gpio_set_alternate_function(pins[i], AF1);
        gpio_set_mode(pins[i], MODER_AF);
        timer_cc_interrupt_register_handler(TIMER1, (timer_interrupt_t) (TIMER_CC1_IRQ + i), cb[i]);
        timer_cc_enable(TIMER1, c, true);             // CCxIE + TIM1_CC line
        timer_set_oc_mode(TIMER1, c, TIMER_OC_TOGGLE);
        timer_set_compare(TIMER1, c, step[i]);
        timer_set_oc_polarity(TIMER1, c, false);
    }
    timer_start(TIMER1);
    while(1);
}
