/**********************************************************************/
/* FMAC (STM32G4) — RM0440 §18                                        */
/**********************************************************************/
#include "libstefi/fmac.h"
#include "libstefi/dma_channel.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"
#include "internal/fmac_internal.h"

#include <stddef.h>


#define PARAM_START   (1u << 31)
#define CR_DMAREN     BIT(8)
#define CR_CLIPEN     BIT(15)
#define CR_RESET      BIT(16)
#define SR_YEMPTY     BIT(0)
#define SR_X1FULL     BIT(1)

enum { FUNC_LOAD_X1 = 1, FUNC_LOAD_X2 = 2, FUNC_LOAD_Y = 3, FUNC_FIR = 8, FUNC_IIR = 9 };

#define DMAMUX_FMAC_READ 110u   /* RM0440 table 91 */

static struct {
    uint8_t channel;
    void (*half)(void);
    void (*done)(void);
} rd;

void fmac_configure(const fmac_config_t *cfg)
{
    RCC->AHB1ENR |= BIT(4);           // FMACEN
    (void) RCC->AHB1ENR;
    FMAC->CR = CR_RESET;
    while (FMAC->CR & CR_RESET);
    FMAC->X1BUFCFG = cfg->x1_base | ((uint32_t) cfg->x1_size << 8) | ((uint32_t) (cfg->x1_full_wm & 3u) << 24);
    FMAC->X2BUFCFG = cfg->x2_base | ((uint32_t) cfg->x2_size << 8);
    FMAC->YBUFCFG  = cfg->y_base  | ((uint32_t) cfg->y_size << 8)  | ((uint32_t) (cfg->y_empty_wm & 3u) << 24);
    FMAC->CR = cfg->clip ? CR_CLIPEN : 0u;
}

static void load(uint32_t func, uint8_t p, uint8_t q, const int16_t *v1, uint8_t n1,
                 const int16_t *v2, uint8_t n2)
{
    FMAC->PARAM = PARAM_START | (func << 24) | p | ((uint32_t) q << 8);
    for (uint32_t i = 0; i < n1; i++) FMAC->WDATA = (uint16_t) v1[i];
    for (uint32_t i = 0; i < n2; i++) FMAC->WDATA = (uint16_t) v2[i];
    while (FMAC->PARAM & PARAM_START);   // cleared when all values are loaded
}

void fmac_load_coefficients(const int16_t *b, uint8_t nb, const int16_t *a, uint8_t na)
{
    load(FUNC_LOAD_X2, nb, na, b, nb, a, na);
}

void fmac_preload_x1(const int16_t *x, uint8_t n) { load(FUNC_LOAD_X1, n, 0, x, n, NULL, 0); }
void fmac_preload_y(const int16_t *y, uint8_t n)  { load(FUNC_LOAD_Y, n, 0, y, n, NULL, 0); }

void fmac_start_filter(uint8_t p, uint8_t q, uint8_t r)
{
    uint32_t func = q ? FUNC_IIR : FUNC_FIR;
    FMAC->PARAM = PARAM_START | (func << 24) | p | ((uint32_t) q << 8) | ((uint32_t) (r & 7u) << 16);
}

void fmac_stop(void)
{
    FMAC->PARAM &= ~PARAM_START;
    FMAC->CR &= ~CR_DMAREN;
    if (rd.channel) dma_channel_disable(PERIPHERAL_DMA1, rd.channel);
}

bool fmac_write(int16_t x)
{
    if (FMAC->SR & SR_X1FULL) return false;
    FMAC->WDATA = (uint16_t) x;
    return true;
}

bool fmac_read(int16_t *y)
{
    if (FMAC->SR & SR_YEMPTY) return false;
    *y = (int16_t) FMAC->RDATA;
    return true;
}

uint32_t fmac_status(void)
{
    return FMAC->SR;
}

static void on_dma(void *aux, uint32_t flags)
{
    (void) aux;
    if ((flags & DMA_CHANNEL_FLAG_HT) && rd.half) rd.half();
    if (flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE)) {
        FMAC->CR &= ~CR_DMAREN;
        dma_channel_disable(PERIPHERAL_DMA1, rd.channel);
        if (rd.done) rd.done();
    }
}

void fmac_read_dma(int16_t *out, uint16_t n, uint8_t channel, void (*half)(void), void (*done)(void))
{
    rd.channel = channel;
    rd.half = half;
    rd.done = done;
    const dma_channel_config_t cfg = {
        .dma = PERIPHERAL_DMA1, .channel = channel, .request = DMAMUX_FMAC_READ,
        .dir = DMA_DIR_PERIPH_TO_MEM,
        .periph_width = DMA_WIDTH_32,   // RDATA: result in the low half-word
        .mem_width = DMA_WIDTH_16,
        .mem_inc = true, .priority = DMA_PRIORITY_HIGH,
        .irq_tc = true, .irq_ht = (half != NULL), .irq_te = true,
    };
    dma_channel_register_handler(PERIPHERAL_DMA1, channel, on_dma, NULL);
    dma_channel_configure(&cfg, &FMAC->RDATA, out, n);
    dma_channel_enable(PERIPHERAL_DMA1, channel);
    FMAC->CR |= CR_DMAREN;
}

/* --------------------------- software model --------------------------- */
static int32_t wrap26(int32_t v)
{
    return (int32_t) ((uint32_t) v << 6) >> 6;
}

uint32_t fmac_model_iir(const int16_t *b, uint8_t p, const int16_t *a, uint8_t q, uint8_t r,
                        const int16_t *x, uint32_t nx, const int16_t *yh, bool clip, int16_t *out)
{
    int16_t hist[64];                 /* y[n-1] at hist[0] .. y[n-q] at hist[q-1] */
    for (uint32_t k = 0; k < q; k++) hist[k] = yh[q - 1u - k];
    uint32_t m = 0;
    for (uint32_t n = (uint32_t) p - 1u; n < nx; n++) {
        int32_t acc = 0;              /* q7.19: q2.30 products >> 11 ... */
        for (uint32_t k = 0; k < p; k++) acc = wrap26(acc + (((int32_t) b[k] * x[n - k]) >> 8));
        for (uint32_t k = 0; k < q; k++) acc = wrap26(acc + (((int32_t) a[k] * hist[k]) >> 8));
        acc = wrap26(acc * (1 << r));
        int32_t v = acc >> 7;         /* ... q1.15 */
        int16_t y = clip ? (int16_t) (v > 32767 ? 32767 : v < -32768 ? -32768 : v) : (int16_t) v;
        for (uint32_t k = q; k > 1u; k--) hist[k - 1u] = hist[k - 2u];
        if (q) hist[0] = y;
        out[m++] = y;
    }
    return m;
}
