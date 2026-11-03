#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* CORDIC co-processor (STM32G4 only) — trigonometric / hyperbolic    */
/* functions in fixed point (q1.31 or q1.15).                         */
/*                                                                    */
/* Arguments are scaled: angles in radians / pi (so q1.31 covers      */
/* -pi..pi), magnitudes default to 1 (0x7FFFFFFF) when only one       */
/* argument is written. 6 iterations give ~2^-19 residual error for   */
/* sine/cosine (RM0440 §17.3.4).                                       */
/**********************************************************************/

typedef enum {
    CORDIC_FUNC_COSINE = 0, CORDIC_FUNC_SINE, CORDIC_FUNC_PHASE, CORDIC_FUNC_MODULUS,
    CORDIC_FUNC_ARCTANGENT, CORDIC_FUNC_HCOSINE, CORDIC_FUNC_HSINE, CORDIC_FUNC_HARCTANGENT,
    CORDIC_FUNC_NATURALLOG, CORDIC_FUNC_SQUAREROOT,
} cordic_function_t;

typedef struct {
    cordic_function_t function;
    uint8_t precision;          /* iterations / 4, 1..15 (6 for q1.31 sin/cos) */
    uint8_t scale;              /* 0..7, function-dependent */
    bool two_args;              /* NARGS: write 2 arguments per calculation */
    bool two_results;           /* NRES: read 2 results per calculation */
    bool q15;                   /* 16-bit arguments/results instead of q1.31 */
} cordic_config_t;

/** Clock on, function configured (aborts any running calculation). */
void cordic_configure(const cordic_config_t *cfg);

/** Polled: one argument word in, one result word out. */
int32_t cordic_calculate(int32_t arg);

/**
 * @brief Calculate @p n results by DMA: arguments memory -> WDATA on one DMA1
 * channel, results RDATA -> memory on another. Non-blocking.
 * @param done  called (interrupt context) when the last result is stored
 * One argument and one result word per calculation (NARGS = NRES = 1 word).
 */
void cordic_calculate_dma(const int32_t *args, int32_t *results, uint16_t n,
                          uint8_t ch_write, uint8_t ch_read, void (*done)(void));
bool cordic_dma_busy(void);
