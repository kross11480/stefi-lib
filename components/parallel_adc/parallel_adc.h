#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "libstefi/interrupts.h"

/*
 * High-speed capture from a parallel-output ADC (LTC1420: 12 bit, 10 MSPS),
 * wired per the board's parallel_adc table (stefi_board.h).
 *
 * How it works: a timer generates the ADC clock as a PWM output, and its
 * update event — once per sample period, half a period away from the
 * clock's active edge — raises a DMA request. The DMA stream copies the
 * GPIO input data register (data pins + overflow flag) into RAM, with no
 * CPU involvement.
 *
 *   F7 (stream DMA): buffers larger than one DMA transfer (65535 items) are
 *   split into equal segments and chained through the stream's double-buffer
 *   mode: while one target fills, the ISR re-points the other one.
 *   L4/G4 (channel DMA): one transfer, so at most 65535 samples per capture
 *   (the L476 has 128 KB RAM — that limit is not the binding one anyway).
 *
 *   parallel_adc_init();
 *   parallel_adc_set_sample_rate(1000000);
 *   parallel_adc_prepare(buf, N, on_done, ctx);  // arm: stream ready, idle
 *   parallel_adc_start();                        // e.g. from a trigger ISR
 *   ... on_done() (interrupt context) ...
 *   bool ovf = parallel_adc_convert(buf, (int16_t *) buf, N);
 *
 * The raw buffer holds GPIO port snapshots; parallel_adc_convert() turns
 * them into signed (or unsigned) sample values.
 */

typedef void (*parallel_adc_done_t)(void *aux);

/** Pins, sample-clock timer (running at max_sample_rate_hz), DMA stream. */
void parallel_adc_init(void);

/**
 * @brief Set the sample rate (clamped to the board's min/max). Ignored
 * while a capture is running.
 * @return the rate actually set: timer_clk / round_down(timer_clk / hz)
 */
uint32_t parallel_adc_set_sample_rate(uint32_t hz);
uint32_t parallel_adc_get_sample_rate(void);

/**
 * @brief Prepare a capture of @p n samples into @p buf (stream left idle).
 *
 * F7: @p n must split into equal segments of <= 65535 samples (at least 2):
 * e.g. any even n up to 131070, or 100000 = 2 x 50000.
 * L4/G4: 1 <= n <= 65535.
 * @param done  called from the DMA interrupt when the buffer is full
 * @return false if n cannot be segmented or a capture is running
 */
bool parallel_adc_prepare(uint16_t *buf, uint32_t n, parallel_adc_done_t done, void *aux);

/** Start the prepared capture with the next sample clock. ISR-safe. */
void parallel_adc_start(void);

/** Capture prepared or running (not yet done/aborted). */
bool parallel_adc_busy(void);
/** Started and still filling. */
bool parallel_adc_running(void);
/** A DMA transfer error ended the last capture early. */
bool parallel_adc_error(void);

void parallel_adc_abort(void);

/**
 * @brief Raw port snapshots -> sample values; @p out may alias @p raw.
 * Signed boards: 2's complement data sign-extended to int16.
 * @return true if the ADC overflow flag was set in any sample
 */
bool parallel_adc_convert(const uint16_t *raw, int16_t *out, uint32_t n);

/** NVIC priority of the capture DMA (default 0, the highest). */
void parallel_adc_set_priority(uint8_t priority);
