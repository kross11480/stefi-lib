/*
 * CORDIC_Sin_DMA (STM32CubeG4, NUCLEO-G431KB) on stefi-lib.
 *
 * The CORDIC computes 64 sines in q1.31 (6 cycles of precision, max.
 * residual error 2^-19). Angles (radians / pi, q1.31) go to the CORDIC by
 * DMA (DMA1 ch1), results come back by DMA (DMA1 ch2), CPU offloaded.
 * The results are checked against double-precision sin().
 *   LED2 on:                     all 64 sines within 2^-19 (0x1000)
 *   LED2 blinking (1 s period):  residual error exceeded
 */
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "g4_example.h"
#include "libstefi/cordic.h"

#define ARRAY_SIZE 64u
#define DELTA      0x00001000   /* 2^-19 in q1.31 */

static int32_t angles[ARRAY_SIZE];
static int32_t calculated_sin[ARRAY_SIZE];
static volatile bool done;

static void on_done(void) { done = true; }

static int32_t ref_sin_q31(int32_t angle_q31)
{
    double s = sin(M_PI * (double) angle_q31 / 2147483648.0) * 2147483648.0;
    if (s > 2147483647.0) s = 2147483647.0;
    if (s < -2147483648.0) s = -2147483648.0;
    return (int32_t) lrint(s);
}

void main(void)
{
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
        if (err > DELTA || err < -DELTA) example_error();
    }
    example_success();
}
