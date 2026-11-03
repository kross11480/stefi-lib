/* Host test: fmac_model_iir() (the bit-exact FMAC model used by the G431
 * FMAC example as its reference) against a double-precision IIR. The
 * model truncates, so it may differ by a few LSB, never more. The model
 * itself was checked once against ST's published reference outputs of
 * FMAC_IIR_PollingToDMA (115/115 identical; ST's data is not shipped here). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "libstefi/fmac.h"
#include "libstefi/dma_channel.h"

/* fmac.c's DMA path is not exercised on the host */
void dma_channel_disable(peripheral_dma_t d, uint8_t c) { (void) d; (void) c; }
void dma_channel_enable(peripheral_dma_t d, uint8_t c) { (void) d; (void) c; }
void dma_channel_register_handler(peripheral_dma_t d, uint8_t c, dma_channel_callback_t f, void *a)
{ (void) d; (void) c; (void) f; (void) a; }
void dma_channel_configure(const dma_channel_config_t *c, volatile const void *p, void *m, uint16_t n)
{ (void) c; (void) p; (void) m; (void) n; }

int main(void)
{
    static const int16_t b[7] = {590, 3540, 8851, 11801, 8851, 3540, 590};
    static const int16_t a[6] = {19448, -29793, 9645, -4884, 671, -81};
    int16_t x[121], y[121], yh[6] = {0};
    for (int n = 0; n < 121; n++)
        x[n] = (int16_t) lrint(32767.0 * (0.5 * sin(2 * 3.141592653589793 * n / 40.0)
                                       + 0.25 * sin(2 * 3.141592653589793 * n / 7.0)));
    uint32_t m = fmac_model_iir(b, 7, a, 6, 0, x, 121, yh, false, y);
    assert(m == 115);

    double hist[6] = {0};
    int worst = 0;
    for (int n = 6; n < 121; n++) {
        double acc = 0;
        for (int k = 0; k < 7; k++) acc += b[k] / 32768.0 * x[n - k] / 32768.0;
        for (int k = 0; k < 6; k++) acc += a[k] / 32768.0 * hist[k];
        for (int k = 5; k > 0; k--) hist[k] = hist[k - 1];
        hist[0] = y[n - 6] / 32768.0;      /* feed back the model's own (q15) output */
        int err = abs((int) lrint(acc * 32768.0) - y[n - 6]);
        if (err > worst) worst = err;
    }
    assert(worst <= 4);
    printf("fmac model: %u outputs, max deviation from float IIR %d LSB\n", m, worst);
    return 0;
}
