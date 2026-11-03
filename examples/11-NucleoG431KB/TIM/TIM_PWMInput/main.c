/*
 * TIM_PWMInput (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * TIM1 at 170 MHz in PWM-input mode on PA9 (TI2, AF6):
 *   IC2 (direct, rising edge)   -> CCR2 = period
 *   IC1 (indirect, falling)     -> CCR1 = high time
 *   slave reset mode on TI2FP2  -> the counter restarts at every rising edge
 * In the CC2 interrupt:
 *   frequency = 170 MHz / CCR2,  duty cycle = CCR1 * 100 / CCR2 %
 * Printed every second (ST-Link VCP). LED2 on: error (no/invalid signal
 * period 0).
 */
#include <stdio.h>

#include "g4_example.h"
#include "libstefi/timer.h"

static volatile uint32_t frequency_hz, duty_percent;

static void on_capture(void *aux)
{
    (void) aux;
    uint32_t period = timer_get_compare(TIMER1, TIMER_CHANNEL2);
    if (period == 0) {
        led_on(LED2);
        frequency_hz = duty_percent = 0;
        return;
    }
    duty_percent = timer_get_compare(TIMER1, TIMER_CHANNEL1) * 100u / period;
    frequency_hz = system_get_core_clock() / period;
}

void main(void)
{
    board_init();

    gpio_set_alternate_function(A9, AF6);
    gpio_set_mode(A9, MODER_AF);
    timer_init(TIMER1);
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL2);                 // IC2 <- TI2, rising
    timer_set_mode_ic(TIMER1, TIMER_CHANNEL1);
    timer_set_ic_source(TIMER1, TIMER_CHANNEL1, true);         // IC1 <- TI2 ...
    timer_set_ic_edge(TIMER1, TIMER_CHANNEL1, TIMER_EDGE_FALLING);   // ... falling
    timer_set_slave_mode(TIMER1, TIMER_SLAVE_RESET, TIMER_TS_TI2FP2);
    timer_cc_interrupt_register_handler(TIMER1, TIMER_CC2_IRQ, on_capture);
    timer_cc_enable(TIMER1, TIMER_CHANNEL1, true);
    timer_set_ic_edge(TIMER1, TIMER_CHANNEL1, TIMER_EDGE_FALLING);   // cc_enable resets the edge
    timer_cc_enable(TIMER1, TIMER_CHANNEL2, true);
    timer_start(TIMER1);

    for (;;) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz, duty cycle: %lu %%\r\n",
               (unsigned long) frequency_hz, (unsigned long) duty_percent);
    }
}
