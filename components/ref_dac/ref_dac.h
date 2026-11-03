#pragma once

#include <stdint.h>

/*
 * 12-bit parallel-input DAC with input latch (LTC1450 and compatibles),
 * wired per the board's ref_dac table (stefi_board.h).
 *
 * The scope uses it to set the ADC reference: Vref = 4.096 V - Vdac, i.e.
 * code = 4095 - Vref_mV. That mapping is the application's business; this
 * driver only moves a 12-bit code onto the pins and latches it.
 */

/** Pins to outputs, /CLR released, code 0 latched (DAC output 0 V). */
void ref_dac_init(void);

/** Latch a new 12-bit code (glitch-free: /LD is held high while the data
 *  pins change, so the output switches once). */
void ref_dac_write(uint16_t code);

/** Last code written. */
uint16_t ref_dac_get(void);

/** Pulse /CLR: output to zero scale. */
void ref_dac_clear(void);
