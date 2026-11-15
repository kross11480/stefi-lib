/*
 * CORDIC_Sin_DMA: port of ST's example (STM32CubeG4, NUCLEO-G431KB) to stefi-lib.
 * 64 sines in q1.31 on the CORDIC (6 cycles), arguments and results by DMA.
 * LED2 on: all within 2^-19 of sin(); blinking: error.
 */

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "board.h"
#include "led.h"

#include <libstefi/systick.h>
#include <libstefi/cordic.h>

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

#define ARRAY_SIZE 64u
#define DELTA      0x00001000   /* 2^-19 in q1.31 */

static int32_t angles[ARRAY_SIZE];
static int32_t calculated_sin[ARRAY_SIZE];
static volatile bool done;

static void on_done(void) { done = true; }

static int32_t ref_sin_q31(int32_t angle_q31) {
    double s = sin(M_PI * (double) angle_q31 / 2147483648.0) * 2147483648.0;
    if (s > 2147483647.0) s = 2147483647.0;
    if (s < -2147483648.0) s = -2147483648.0;
    return (int32_t) lrint(s);
}

void main() {
    board_init();

    // angles 0, 1/32, 2/32 ... of pi; past 1.0 the q1.31 value wraps to -1
    for (uint32_t i = 0; i < ARRAY_SIZE; i++) angles[i] = (int32_t) (i * 0x04000000u);

    const cordic_config_t cfg = {
        .function = CORDIC_FUNC_SINE, .precision = 6, .scale = 0,
        .two_args = false, .two_results = false, .q15 = false,
    };
    cordic_configure(&cfg);
    cordic_calculate_dma(angles, calculated_sin, ARRAY_SIZE, 1, 2, on_done);
    while (!done);

    for (uint32_t i = 0; i < ARRAY_SIZE; i++) {
        int64_t err = (int64_t) calculated_sin[i] - ref_sin_q31(angles[i]);
        if (err > DELTA || err < -DELTA) led_error_forever();
    }
    led_success_forever();
}
