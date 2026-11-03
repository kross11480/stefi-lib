#include "board.h"

#include "button.h"
#include "led.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

/*
 * Board-specific configuration with default peripheral mapping
 * NUCLEO-F767ZI + USB-oscilloscope front-end
 *
 * Note (timer budget, all timer kernels at 100 MHz):
 *  Systick  - delay functions
 *  Timer 1  - ADC sample clock (CH1 PWM on PE9) + DMA request per sample
 *  Timer 3  - button debouncing (10 ms one-shot)
 * DMA budget:
 *  DMA2 stream 5 / channel 6 (TIM1_UP) - ADC capture (parallel_adc)
 *  DMA1 stream 3 / channel 4 (USART3_TX) - uart_dma_out()
 */

led_config_t leds[NUM_LEDS] = {
    [LED_GREEN] = {B0,  LOW},
    [LED_BLUE]  = {B7,  LOW},
    [LED_RED]   = {B14, LOW},
};

button_config_t buttons[NUM_BUTTONS] = {
    [BUTTON_USER] = {C13, PULL_DOWN},   // board has its own pull-down; active high
};

_Static_assert(NUM_BUTTONS <= STEFI_MAX_BUTTONS,
        "button.c sizes its per-button state by STEFI_MAX_BUTTONS");

hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER] = {
    // 100 MHz / 10000 = 10 kHz tick, 100 ticks = 10 ms
    [DEBOUNCE_TIMER] = {TIMER3, 10000, 100},
};

parallel_adc_config_t parallel_adc = {
    .data_lsb_pin = F3,        // D0
    .data_bits = 12,           // D0..D11 = PF3..PF14
    .overflow_pin = F15,       // OF
    .signed_output = true,     // LTC1420 in 2's complement mode
    .clock_pin = E9,
    .clock_af = AF1,
    .clock_timer = TIMER1,
    .clock_channel = 1,
    .timer_clk_hz = BOARD_TIMER_CLK,
    .min_sample_rate_hz = 20000,      // LTC1420 datasheet minimum
    .max_sample_rate_hz = 10000000,   // LTC1420: 10 MSPS
    .dma = PERIPHERAL_DMA2,
    .dma_stream = 5,
    .dma_channel = 6,          // TIM1_UP
};

ref_dac_config_t ref_dac = {
    .data = {E2, E3, E4, E5, E6, E7, E8, E10, E11, E12, E13, E14},   // D0..D11
    .clear_n = E0,
    .load_n = E1,
    .otype = OPEN_DRAIN,       // external pull-ups to 5 V
};

edge_trigger_config_t edge_trigger = {
    .portpin = F2,
    .pull = PULL_DOWN,
};

void board_init() {
    //200 MHz core, 100 MHz timers, 48 MHz USB
    system_init();

    //UART (USART3/PD8/PD9 — ST-Link VCP), HSI-clocked
    uart_configure();

    //initialize all leds, default, output not PWM
    for(uint32_t i = 0; i < NUM_LEDS; i++) {
        led_init(i, OUTPUT);
    }

    for(uint32_t i = 0; i < NUM_BUTTONS; i++) {
        button_init(i);
    }

    //intialize default timers
    for(uint32_t i = 0; i < NUM_RESERVED_TIMER; i++) {
        timer_init(hardware_timers[i].timer);
        timer_set_period(hardware_timers[i].timer, hardware_timers[i].prescaler, hardware_timers[i].period);
    }

    systick_init(BOARD_SYSCLK);
    systick_start();
}
