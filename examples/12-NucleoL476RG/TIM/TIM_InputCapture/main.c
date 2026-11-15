/*
 * TIM_InputCapture: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Frequency on PA9 (TIM1 CH2 input capture), printed every second.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

#include <stdio.h>

static volatile uint32_t frequency_hz;

static void on_capture(void *aux) {
    (void) aux;
    static uint32_t first, have;
    uint32_t now = timer_get_compare(TIMER1, TIMER_CHANNEL2);
    if (!have) { first = now; have = 1; return; }
    uint32_t diff = (now - first) & 0xFFFFu;
    if (diff) frequency_hz = system_get_core_clock() / diff;
    have = 0;
}

void main() {
    board_init();
    gpio_set_alternate_function(A9, AF1);
    gpio_set_mode(A9, MODER_AF);
    timer_init(TIMER1);
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL2);
    timer_cc_interrupt_register_handler(TIMER1, TIMER_CC2_IRQ, on_capture);
    timer_cc_enable(TIMER1, TIMER_CHANNEL2, true);
    timer_start(TIMER1);
    while(1) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz\r\n", (unsigned long) frequency_hz);
    }
}
