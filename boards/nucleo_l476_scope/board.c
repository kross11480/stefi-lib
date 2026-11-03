#include "board.h"

#include "button.h"
#include "led.h"

#include <libstefi/system.h>
#include <libstefi/systick.h>
#include <libstefi/uart.h>

/*
 * Board-specific configuration with default peripheral mapping
 * NUCLEO-L476RG + USB-oscilloscope front-end
 *
 * Note (timer budget, all timer kernels at 80 MHz):
 *  Systick  - delay functions
 *  Timer 1  - ADC sample clock (CH1 PWM on PA8) + DMA request per sample
 *  Timer 6  - button debouncing (10 ms)
 * DMA budget:
 *  DMA1 channel 6 / CSELR 7 (TIM1_UP) - ADC capture (parallel_adc)
 *  DMA1 channel 7 / CSELR 2 (USART2_TX) - uart_dma_out()
 */

led_config_t leds[NUM_LEDS] = {
    [LED_GREEN] = {A5, LOW},
};

button_config_t buttons[NUM_BUTTONS] = {
    [BUTTON_USER] = {C13, PULL_UP},
};

_Static_assert(NUM_BUTTONS <= STEFI_MAX_BUTTONS,
        "button.c sizes its per-button state by STEFI_MAX_BUTTONS");

hardware_timer_config_t hardware_timers[NUM_RESERVED_TIMER] = {
    // 80 MHz / 8000 = 10 kHz tick, 100 ticks = 10 ms
    [DEBOUNCE_TIMER] = {TIMER6, 8000, 100},
};

parallel_adc_config_t parallel_adc = {
    .data_lsb_pin = C0,        // D0
    .data_bits = 12,           // D0..D11 = PC0..PC11
    .overflow_pin = C12,       // OF
    .signed_output = true,     // LTC1420 in 2's complement mode
    .clock_pin = A8,
    .clock_af = AF1,
    .clock_timer = TIMER1,
    .clock_channel = 1,
    .timer_clk_hz = BOARD_TIMER_CLK,
    .min_sample_rate_hz = 20000,      // LTC1420 datasheet minimum
    /* One DMA beat per sample over AHB2 -> SRAM at 80 MHz: 5 MSPS
     * (16 timer ticks per sample) is a conservative ceiling, not measured.
     * The LTC1420 itself does 10 MSPS. */
    .max_sample_rate_hz = 5000000,
    .dma = PERIPHERAL_DMA1,
    .dma_channel = 6,          // RM0351 table 41: TIM1_UP = DMA1 ch6,
    .dma_request = 7,          //                  C6S = 7
};

ref_dac_config_t ref_dac = {
    .data = {B0, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11},   // D0..D11
    .clear_n = B12,
    .load_n = B13,
    .otype = OPEN_DRAIN,       // pull-ups on the front-end
};

edge_trigger_config_t edge_trigger = {
    .portpin = A0,
    .pull = PULL_DOWN,
};

void board_init() {
    //80 MHz core and timers (USB's 48 MHz MSI is started by usb_cdc_init)
    system_init();

    //UART (USART2/PA2/PA3 — ST-Link VCP), HSI16-clocked
    uart_configure();

    for(uint32_t i = 0; i < NUM_LEDS; i++) {
        led_init(i, OUTPUT);
    }

    for(uint32_t i = 0; i < NUM_BUTTONS; i++) {
        button_init(i);
    }

    for(uint32_t i = 0; i < NUM_RESERVED_TIMER; i++) {
        timer_init(hardware_timers[i].timer);
        timer_set_period(hardware_timers[i].timer, hardware_timers[i].prescaler, hardware_timers[i].period);
    }

    systick_init(BOARD_SYSCLK);
    systick_start();
}
