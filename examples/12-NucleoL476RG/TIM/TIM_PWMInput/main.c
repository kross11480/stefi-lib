/* TIM_PWMInput (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * TIM3 at 80 MHz in PWM-input mode on PC7 (TI2, AF2): IC2 direct rising
 * (period), IC1 indirect falling (high time), slave reset on TI2FP2.
 * CC2 interrupt: frequency = 80 MHz / CCR2, duty = CCR1 * 100 / CCR2 %.
 * Printed every second on the VCP; LD2 on: error (period 0). */
#include <stdio.h>

#include "l4_example.h"
#include "libstefi/timer.h"

static volatile uint32_t frequency_hz, duty_percent;

static void on_capture(void *aux)
{
    (void) aux;
    uint32_t period = timer_get_compare(TIMER3, TIMER_CHANNEL2);
    if (!period) { led_on(LED2); return; }
    duty_percent = timer_get_compare(TIMER3, TIMER_CHANNEL1) * 100u / period;
    frequency_hz = system_get_core_clock() / period;
}

void main(void)
{
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
    for (;;) {
        systick_delay_ms(1000);
        printf("frequency: %lu Hz, duty cycle: %lu %%\r\n", (unsigned long) frequency_hz, (unsigned long) duty_percent);
    }
}
