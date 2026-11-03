#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* FMAC filter accelerator (STM32G4 only) — FIR/IIR in q1.15.         */
/*                                                                    */
/* The FMAC has 256 x 16-bit local memory, split by the application  */
/* into three circular buffers: X1 (input samples), X2 (coefficients: */
/* B feed-forward, then A feedback) and Y (output samples).           */
/*                                                                    */
/* IIR direct form 1:                                                 */
/*   y[n] = 2^R * ( sum_{k=0}^{P-1} b_k x[n-k] + sum_{k=1}^{Q} a_k y[n-k] ) */
/* Products are q2.30, truncated into a 26-bit q7.19 accumulator; the */
/* output takes the q1.15 bits (wrap, or saturate with clipping on).  */
/* fmac_model_iir() reproduces this in software.                      */
/**********************************************************************/

typedef struct {
    uint8_t x1_base, x1_size;     /* input buffer */
    uint8_t x2_base, x2_size;     /* coefficients */
    uint8_t y_base, y_size;       /* output buffer */
    uint8_t x1_full_wm;           /* watermark code: 0 = 1, 1 = 2, 2 = 4, 3 = 8 free */
    uint8_t y_empty_wm;           /* watermark code: 0 = 1, 1 = 2, 2 = 4, 3 = 8 ready */
    bool clip;                    /* saturate instead of wrapping */
} fmac_config_t;

/** Clock on, reset, buffers configured. */
void fmac_configure(const fmac_config_t *cfg);

/** Load coefficients: @p nb feed-forward B, then @p na feedback A (na may be 0). */
void fmac_load_coefficients(const int16_t *b, uint8_t nb, const int16_t *a, uint8_t na);
/** Preload the input buffer X1 / the output history Y (polling). */
void fmac_preload_x1(const int16_t *x, uint8_t n);
void fmac_preload_y(const int16_t *y, uint8_t n);

/** Start an IIR (q > 0) or FIR (q == 0) with p taps, gain 2^r. Runs until stopped. */
void fmac_start_filter(uint8_t p, uint8_t q, uint8_t r);
void fmac_stop(void);

/** Polled access: write one input / read one output (false if full/empty). */
bool fmac_write(int16_t x);
bool fmac_read(int16_t *y);

/**
 * @brief Read @p n outputs by DMA (RDATA -> memory) on a DMA1 channel.
 * @param half  optional, called at n/2 (interrupt context)
 * @param done  called when n outputs are stored (interrupt context)
 */
void fmac_read_dma(int16_t *out, uint16_t n, uint8_t channel, void (*half)(void), void (*done)(void));

/** SR flags: bit 0 YEMPTY, 1 X1FULL, 8 OVFL, 9 UNFL, 10 SAT. */
uint32_t fmac_status(void);

/**
 * @brief Software model of the FMAC IIR (bit-exact accumulator arithmetic),
 * for reference outputs: processes x[0..nx-1] like a preloaded X1 buffer of
 * that length, starting from y-history yh[0..q-1] (oldest first), and writes
 * nx - p + 1 outputs. Coefficients as for fmac_load_coefficients().
 * @return number of outputs written
 */
uint32_t fmac_model_iir(const int16_t *b, uint8_t p, const int16_t *a, uint8_t q, uint8_t r,
                        const int16_t *x, uint32_t nx, const int16_t *yh, bool clip, int16_t *out);
