/*
 * DAC_SignalsGeneration: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * DAC ch1 (PA4), TIM6-triggered; B1 switches between a triangle wave and an
 * escalator streamed by DMA.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/dac.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

static const uint16_t escalator[6] = {0x000, 0x330, 0x660, 0x990, 0xCC0, 0xFF0};
static volatile bool key_pressed;
static volatile uint8_t waveform;      /* 0 escalator, 1 triangle */

static void on_button(void *aux) {
    (void) aux;
    key_pressed = true;
    waveform ^= 1u;
}

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

    timer_init(TIMER6);
    timer_set_period(TIMER6, 1, 0x800);
    timer_set_master_mode(TIMER6, TIMER_TRGO_UPDATE, false);
    timer_start(TIMER6);

    while(1) {
        if (!key_pressed) continue;
        key_pressed = false;
        dac_stop_dma(1);
        dac_disable(1);
        if (waveform == 1) {
            dac_init(1, DAC_MODE_NORMAL_BUFFERED, DAC_TRIGGER_TIM6);
            dac_triangle(1, 9);                   // amplitude 2^10 - 1
            dac_write(1, 0x100);
        } else {
            dac_init(1, DAC_MODE_NORMAL_BUFFERED, DAC_TRIGGER_TIM6);
            dac_start_dma(1, escalator, 6);
        }
    }
}
