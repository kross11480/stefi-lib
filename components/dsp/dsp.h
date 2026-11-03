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
