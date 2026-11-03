#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**********************************************************************/
/* CRC calculation unit (STM32L4 / STM32G4)                           */
/*                                                                    */
/* Programmable polynomial of 7, 8, 16 or 32 bits, initial value,     */
/* input reversal (per byte / half-word / word) and output reversal.  */
/* Data are fed in bytes, half-words or words; the CRC continues      */
/* across calls until crc_reset().                                    */
/* crc_sw() computes the same CRC in software (reference values).    */
/**********************************************************************/

typedef enum { CRC_REV_IN_NONE = 0, CRC_REV_IN_BYTE, CRC_REV_IN_HALFWORD, CRC_REV_IN_WORD } crc_rev_in_t;

typedef struct {
    uint32_t polynomial;    /* e.g. 0x04C11DB7 (default), 0x1021, 0x65 */
    uint8_t width;          /* 7, 8, 16 or 32 */
    uint32_t init;          /* initial value (default 0xFFFFFFFF) */
    crc_rev_in_t rev_in;
    bool rev_out;
} crc_config_t;

/** Ethernet CRC-32 settings of the unit's reset state (poly 0x04C11DB7, init ~0). */
#define CRC_CONFIG_DEFAULT ((crc_config_t) {0x04C11DB7u, 32, 0xFFFFFFFFu, CRC_REV_IN_NONE, false})

void crc_configure(const crc_config_t *cfg);
/** Load the initial value again (start a new CRC). */
void crc_reset(void);

uint32_t crc_feed_bytes(const uint8_t *data, size_t n);
uint32_t crc_feed_halfwords(const uint16_t *data, size_t n);
uint32_t crc_feed_words(const uint32_t *data, size_t n);
uint32_t crc_value(void);

/**
 * @brief Software model of the unit: CRC of @p n items of @p item_bits
 * (8, 16, 32) starting from @p crc (pass cfg->init for a new CRC).
 */
uint32_t crc_sw(const crc_config_t *cfg, uint32_t crc, const void *data, size_t n, uint8_t item_bits);
