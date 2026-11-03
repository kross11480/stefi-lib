/* OPAMP_PGA (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * DAC channel 2 generates a sine (32 samples by DMA, DMA1 ch4, triggered
 * by TIM2 TRGO); OPAMP2 in programmable-gain mode amplifies it (input:
 * the DAC channel internally), output on PB0 (OPAMP2_VOUT). Each B1 press
 * steps: gain 2 -> gain 4 -> gain 2 low-power -> gain 4 low-power -> ...
 * — gain and power mode change on the fly while the OPAMP stays enabled.
 * Compare PA5 (DAC) and PB0 on an oscilloscope. */
#include <math.h>

#include "l4_example.h"
#include "libstefi/dac.h"
#include "libstefi/opamp.h"
#include "libstefi/timer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define N 32
static uint16_t sine[N];
static volatile bool key_pressed;

static void on_button(void *aux) { (void) aux; key_pressed = true; }

void main(void)
{
    board_init();
    example_button_irq(on_button);

    // small sine around mid-scale: x4 still fits in 3.3 V
    for (int i = 0; i < N; i++) sine[i] = (uint16_t) (512 + 300 * sin(2 * M_PI * i / N));

    timer_init(TIMER2);
    timer_set_period(TIMER2, 1, 0x1000);
    timer_set_master_mode(TIMER2, TIMER_TRGO_UPDATE, false);
    timer_start(TIMER2);
    dac_init(2, DAC_MODE_NORMAL_BUFFERED, DAC_TRIGGER_TIM2);
    dac_start_dma(2, sine, N);

    opamp_config_t op = {.instance = 2, .mode = OPAMP_MODE_PGA, .vinp_dac = true, .pga_gain_log2 = 1};
    opamp_init(&op);

    uint8_t step = 0;
    for (;;) {
        if (!key_pressed) continue;
        key_pressed = false;
        step = (step + 1u) % 4u;
        opamp_set_gain(2, (step & 1u) ? 2 : 1);      // x4 : x2
        opamp_set_low_power(2, step >= 2);
        led_toggle(LED2);
    }
}
