/*
 * TIM_InputCapture: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
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
    static uint32_t first, have_first;
    uint32_t now = timer_get_compare(TIMER1, TIMER_CHANNEL2);
    if (!have_first) {
        first = now;
        have_first = 1;
        return;
    }
    uint32_t diff = (now - first) & 0xFFFFu;   // counter wraps at 0xFFFF
    if (diff) frequency_hz = system_get_core_clock() / diff;
    have_first = 0;
}

void main() {
    board_init();

    gpio_set_alternate_function(A9, AF6);
    gpio_set_mode(A9, MODER_AF);
    timer_init(TIMER1);
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL2);          // PSC 0, ARR 0xFFFF, TI2
    timer_set_ic_edge(TIMER1, TIMER_CHANNEL2, TIMER_EDGE_RISING);
    timer_cc_interrupt_register_handler(TIMER1, TIMER_CC2_IRQ, on_capture);
    timer_cc_enable(TIMER1, TIMER_CHANNEL2, true);      // CC2 interrupt (TIM1_CC)
    timer_start(TIMER1);

    while(1) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz\r\n", (unsigned long) frequency_hz);
    }
}
