/*
 * TIM_ExtTriggerSynchro: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * TIM2 (PA0), TIM3 (PB4), TIM4 (PB6) toggle outputs, all gated by PB3.
 */

#include "board.h"
#include "led.h"

#include <libstefi/gpio.h>
#include <libstefi/peripheral.h>
#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

static void toggle_ch1(tim_id_t tim, gpio_id_t pin, afr_t af) {
    peripheral_gpio_enable(gpio_get_port_from_portpin(pin));
    gpio_set_alternate_function(pin, af);
    gpio_set_mode(pin, MODER_AF);
    timer_init(tim);
    timer_set_period(tim, (uint16_t) (system_get_core_clock() / 16000000u), 80);
    timer_set_oc_mode(tim, TIMER_CHANNEL1, TIMER_OC_TOGGLE);
    timer_set_compare(tim, TIMER_CHANNEL1, 40);
    timer_cc_enable(tim, TIMER_CHANNEL1, false);
    timer_set_oc_polarity(tim, TIMER_CHANNEL1, true);
}

void main() {
    board_init();

    toggle_ch1(TIMER2, A0, AF1);
    peripheral_gpio_enable(PERIPHERAL_GPIOB);
    gpio_set_alternate_function(B3, AF1);            // TIM2_CH2 = trigger input
    gpio_set_mode(B3, MODER_AF);
    timer_set_ic_source(TIMER2, TIMER_CHANNEL2, false);
    timer_set_slave_mode(TIMER2, TIMER_SLAVE_GATED, TIMER_TS_TI2FP2);
    timer_set_master_mode(TIMER2, TIMER_TRGO_ENABLE, true);

    toggle_ch1(TIMER3, B4, AF2);
    timer_set_slave_mode(TIMER3, TIMER_SLAVE_GATED, TIMER_TS_ITR1);
    timer_set_master_mode(TIMER3, TIMER_TRGO_ENABLE, true);

    toggle_ch1(TIMER4, B6, AF2);
    timer_set_slave_mode(TIMER4, TIMER_SLAVE_GATED, TIMER_TS_ITR2);

    timer_start(TIMER4);
    timer_start(TIMER3);
    timer_start(TIMER2);
    while(1);
}
