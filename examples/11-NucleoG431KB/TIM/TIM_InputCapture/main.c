/*
 * TIM_InputCapture (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * TIM1 at 170 MHz (prescaler 1, period 0xFFFF), CH2 = input capture on the
 * rising edges of the external signal at PA9 (AF6). The CC2 interrupt
 * takes the difference of two consecutive captures:
 *   frequency = 170 MHz / difference
 * Range ~2.6 kHz (170 MHz / 65535) to a few 100 kHz (interrupt time).
 * The measured frequency is printed every second (ST-Link VCP, 115200).
 */
#include <stdio.h>

#include "g4_example.h"
#include "libstefi/timer.h"

static volatile uint32_t frequency_hz;

static void on_capture(void *aux)
{
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

void main(void)
{
    board_init();

    gpio_set_alternate_function(A9, AF6);
    gpio_set_mode(A9, MODER_AF);
    timer_init(TIMER1);
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL2);          // PSC 0, ARR 0xFFFF, TI2
    timer_set_ic_edge(TIMER1, TIMER_CHANNEL2, TIMER_EDGE_RISING);
    timer_cc_interrupt_register_handler(TIMER1, TIMER_CC2_IRQ, on_capture);
    timer_cc_enable(TIMER1, TIMER_CHANNEL2, true);      // CC2 interrupt (TIM1_CC)
    timer_start(TIMER1);

    for (;;) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz\r\n", (unsigned long) frequency_hz);
    }
}
