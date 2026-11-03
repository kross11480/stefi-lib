#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "libstefi/interrupts.h"

/**********************************************************************/
/* Low-power timer LPTIM1 (STM32L4) — keeps running in Stop 0/1/2.    */
/*   IN1 PB5 (AF1), OUT PB2 (AF1)                                     */
/**********************************************************************/

typedef enum { LPTIM_CLK_PCLK = 0, LPTIM_CLK_LSI = 1, LPTIM_CLK_HSI16 = 2, LPTIM_CLK_LSE = 3 } lptim_kernel_clock_t;

typedef enum {
    LPTIM_COUNT_INTERNAL = 0,     /* count the kernel clock */
    LPTIM_COUNT_IN1_SAMPLED,      /* count IN1 rising edges, sampled by the kernel clock */
    LPTIM_COUNT_IN1_CLOCK,        /* IN1 is the counter clock (works without a kernel clock) */
} lptim_count_t;

/** Clock on, kernel clock selected, counter source, prescaler 2^presc_log2 (0..7). */
void lptim_init(lptim_kernel_clock_t clk, lptim_count_t count, uint8_t presc_log2);

/** PWM on OUT: period = arr + 1 counts, output high while CNT > cmp (polarity high). */
void lptim_pwm_start(uint16_t arr, uint16_t cmp);
/** Continuous counting with an interrupt each time CNT reaches arr. */
void lptim_counter_start(uint16_t arr, callbackfn_t on_arr_match, void *aux);
void lptim_stop(void);
uint16_t lptim_count(void);
