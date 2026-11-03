/* DAC_SignalsGeneration (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * DAC channel 1 (PA4), triggered by TIM6 TRGO (80 MHz / 2048 = 39 kHz).
 * Each B1 press switches between:
 *   - a triangle wave: hardware generator, amplitude 1023 on top of 0x100
 *     (~1 V, tens of Hz)
 *   - an escalator: 6 steps streamed from memory by DMA (DMA1 ch3, circular,
 *     ~3 V, kHz) */
#include "l4_example.h"
#include "libstefi/dac.h"
#include "libstefi/timer.h"

static const uint16_t escalator[6] = {0x000, 0x330, 0x660, 0x990, 0xCC0, 0xFF0};
static volatile bool key_pressed;
static volatile uint8_t waveform;      /* 0 escalator, 1 triangle */

static void on_button(void *aux)
{
    (void) aux;
    key_pressed = true;
    waveform ^= 1u;
}

void main(void)
{
    board_init();
    example_button_irq(on_button);

    timer_init(TIMER6);
    timer_set_period(TIMER6, 1, 0x800);
    timer_set_master_mode(TIMER6, TIMER_TRGO_UPDATE, false);
    timer_start(TIMER6);

    for (;;) {
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
