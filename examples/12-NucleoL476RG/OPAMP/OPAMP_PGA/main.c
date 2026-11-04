/*
 * OPAMP_PGA: port of ST's example (STM32CubeL4, NUCLEO-L476RG) to stefi-lib.
 * DAC ch2 sine amplified by OPAMP2 in PGA mode (out PB0); B1 steps gain 2/4,
 * normal/low power.
 */

#include "board.h"
#include "led.h"
#include "button.h"

#include <libstefi/dac.h>
#include <libstefi/opamp.h>
#include <libstefi/systick.h>
#include <libstefi/timer.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define N 32
static uint16_t sine[N];
static volatile bool key_pressed;

static void on_button(void *aux) { (void) aux; key_pressed = true; }

void main() {
    board_init();
    button_interrupt_init(BUTTON_USER, on_button);

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
    while(1) {
        if (!key_pressed) continue;
        key_pressed = false;
        step = (step + 1u) % 4u;
        opamp_set_gain(2, (step & 1u) ? 2 : 1);      // x4 : x2
        opamp_set_low_power(2, step >= 2);
        led_toggle(LED2);
    }
}
