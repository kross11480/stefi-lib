/*
 * TIM_PWMInput: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * Frequency and duty cycle on PC7 (TIM3 PWM-input mode), printed every second.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

#include <stdio.h>

static volatile uint32_t frequency_hz, duty_percent;

static void on_capture(void *aux) {
    (void) aux;
    uint32_t period = timer_get_compare(TIMER3, TIMER_CHANNEL2);
    if (!period) { led_on(LED2); return; }
    duty_percent = timer_get_compare(TIMER3, TIMER_CHANNEL1) * 100u / period;
    frequency_hz = system_get_core_clock() / period;
}

void main() {
    board_init();
    gpio_set_alternate_function(C7, AF2);
    gpio_set_mode(C7, MODER_AF);
    timer_init(TIMER3);
    timer_set_mode_ic(TIMER3, TIMER_CHANNEL2);
    timer_set_mode_ic(TIMER3, TIMER_CHANNEL1);
    timer_set_ic_source(TIMER3, TIMER_CHANNEL1, true);
    timer_set_slave_mode(TIMER3, TIMER_SLAVE_RESET, TIMER_TS_TI2FP2);
    timer_cc_interrupt_register_handler(TIMER3, TIMER_CC2_IRQ, on_capture);
    timer_cc_enable(TIMER3, TIMER_CHANNEL1, true);
    timer_set_ic_edge(TIMER3, TIMER_CHANNEL1, TIMER_EDGE_FALLING);
    timer_cc_interrupt(TIMER3, TIMER_CHANNEL1, false);
    timer_cc_enable(TIMER3, TIMER_CHANNEL2, true);
    timer_start(TIMER3);
    while(1) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz, duty cycle: %lu %%\r\n", (unsigned long) frequency_hz, (unsigned long) duty_percent);
    }
}
