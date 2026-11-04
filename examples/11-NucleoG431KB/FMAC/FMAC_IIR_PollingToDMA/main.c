/*
 * FMAC_IIR_PollingToDMA: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * IIR filter (7 + 6 taps) on the FMAC: preload by polling, outputs by DMA,
 * checked against fmac_model_iir(). LED2 on: identical; blinking: error.
 */

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "board.h"
#include "led.h"

#include <libstefi/systick.h>
#include <libstefi/util.h>
#include <libstefi/fmac.h>

/* LED2 blinking with a 1 s period: error */
static void led_error_forever() {
    while(1) {
        led_toggle(LED2);
        systick_delay_ms(500);
    }
}

/* LED2 on: passed */
static void led_success_forever() {
    led_on(LED2);
    while(1);
}

#define P  7     /* feed-forward taps */
#define Q  6     /* feedback taps */
#define R  0     /* gain 2^0 */
#define D1 114
#define D2 115
#define COEFF_BASE  0
#define COEFF_SIZE  (P + Q)
#define INPUT_BASE  COEFF_SIZE
#define INPUT_SIZE  (P + D1)
#define OUTPUT_BASE (INPUT_BASE + INPUT_SIZE)
#define OUTPUT_SIZE (Q + D2)

/* q1.15 coefficients of ST's example (6th-order low-pass) */
static const int16_t coeff_b[P] = {590, 3540, 8851, 11801, 8851, 3540, 590};
static const int16_t coeff_a[Q] = {19448, -29793, 9645, -4884, 671, -81};

static int16_t input[INPUT_SIZE];
static const int16_t y_preload[Q] = {0};
static int16_t calculated[D2];
static int16_t expected[D2];
static volatile bool half_ready, all_ready;

static void on_half(void) { half_ready = true; }
static void on_done(void) { all_ready = true; }

void main() {
    board_init();

    for (uint32_t n = 0; n < INPUT_SIZE; n++) {
        double v = 0.5 * sin(2.0 * M_PI * n / 40.0) + 0.25 * sin(2.0 * M_PI * n / 7.0);
        input[n] = (int16_t) lrint(v * 32767.0);
    }

    const fmac_config_t cfg = {
        .x1_base = INPUT_BASE,  .x1_size = INPUT_SIZE,
        .x2_base = COEFF_BASE,  .x2_size = COEFF_SIZE,
        .y_base  = OUTPUT_BASE, .y_size  = OUTPUT_SIZE,
        .x1_full_wm = 0, .y_empty_wm = 0,       // thresholds = 1
        .clip = false,
    };
    fmac_configure(&cfg);
    fmac_load_coefficients(coeff_b, P, coeff_a, Q);
    fmac_preload_x1(input, INPUT_SIZE);
    fmac_preload_y(y_preload, Q);

    fmac_read_dma(calculated, D2, 1, on_half, on_done);
    fmac_start_filter(P, Q, R);
    while (!half_ready);
    while (!all_ready);
    fmac_stop();

    if (fmac_status() & (BIT(8) | BIT(9) | BIT(10))) led_error_forever();   // OVFL UNFL SAT

    uint32_t n = fmac_model_iir(coeff_b, P, coeff_a, Q, R, input, INPUT_SIZE, y_preload, false, expected);
    if (n != D2) led_error_forever();
    for (uint32_t i = 0; i < D2; i++) {
        if (calculated[i] != expected[i]) led_error_forever();
    }
    led_success_forever();
}
