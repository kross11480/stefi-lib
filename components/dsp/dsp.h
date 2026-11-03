#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Small fixed-point signal helpers.
 */

/**
 * @brief Centred moving-average filter (recursive running sum, O(n)).
 *
 * out[i] = mean(in[i - w/2 .. i + w/2]) for every i where the window fits;
 * the (w-1)/2 samples at either edge, where it does not, are copied from the
 * input unfiltered. The sum is kept in 32 bits, so any int16 input is safe
 * for windows up to 65535.
 *
 * in and out must not overlap.
 *
 * @param window  odd number of points, 1 <= window <= n
 * @return false (out untouched) if window is even, zero or larger than n
 */
bool dsp_moving_average_i16(const int16_t *in, int16_t *out, uint32_t n, uint32_t window);

/** Widest window dsp_moving_average_i16_inplace() supports. */
#define DSP_MOVAVG_INPLACE_MAX_WINDOW 1025u

/**
 * @brief Same filter as dsp_moving_average_i16(), overwriting @p buf.
 *
 * Needs no second sample buffer (for RAM-limited MCUs such as the L476):
 * the (window+1)/2 original values still needed after being overwritten are
 * kept in a small internal history ring. Not reentrant.
 *
 * @return false (buf untouched) if window is even, zero, larger than n or
 *         larger than DSP_MOVAVG_INPLACE_MAX_WINDOW
 */
bool dsp_moving_average_i16_inplace(int16_t *buf, uint32_t n, uint32_t window);
