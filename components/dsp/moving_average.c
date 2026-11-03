#include "dsp.h"

bool dsp_moving_average_i16(const int16_t *in, int16_t *out, uint32_t n, uint32_t window)
{
    if (window == 0 || (window % 2u) == 0 || window > n) return false;

    uint32_t half = (window - 1u) / 2u;
    int32_t sum = 0;

    for (uint32_t i = 0; i < window; i++) sum += in[i];

    for (uint32_t i = 0; i < half; i++) out[i] = in[i];
    out[half] = (int16_t) (sum / (int32_t) window);
    for (uint32_t i = half + 1u; i + half < n; i++) {
        sum += in[i + half] - in[i - half - 1u];
        out[i] = (int16_t) (sum / (int32_t) window);
    }
    for (uint32_t i = n - half; i < n; i++) out[i] = in[i];
    return true;
}
