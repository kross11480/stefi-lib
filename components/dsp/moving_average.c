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

bool dsp_moving_average_i16_inplace(int16_t *buf, uint32_t n, uint32_t window)
{
    /* history of the originals of the last half+1 positions written */
    static int16_t hist[(DSP_MOVAVG_INPLACE_MAX_WINDOW + 1u) / 2u];

    if (window == 0 || (window % 2u) == 0 || window > n
            || window > DSP_MOVAVG_INPLACE_MAX_WINDOW) return false;

    uint32_t half = (window - 1u) / 2u;
    uint32_t hlen = half + 1u;
    int32_t sum = 0;

    for (uint32_t i = 0; i < window; i++) sum += buf[i];

    /* buf[0..half-1] keep their values (edges unfiltered); their originals
     * are still in place when the window slides past them. Position i's
     * original goes to hist[i % hlen] just before it is overwritten. */
    hist[half % hlen] = buf[half];
    buf[half] = (int16_t) (sum / (int32_t) window);
    for (uint32_t i = half + 1u; i + half < n; i++) {
        uint32_t old = i - half - 1u;   /* leaving the window */
        int16_t leaving = (old >= half) ? hist[old % hlen] : buf[old];
        sum += buf[i + half] - leaving;
        hist[i % hlen] = buf[i];
        buf[i] = (int16_t) (sum / (int32_t) window);
    }
    /* tail buf[n-half..n-1] untouched = unfiltered edge */
    return true;
}
