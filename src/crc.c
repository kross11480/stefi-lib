/**********************************************************************/
/* CRC (STM32L4 RM0351 §14 / STM32G4 RM0440 §16) — same unit          */
/**********************************************************************/
#include "libstefi/crc.h"
#include "libstefi/util.h"
#include "internal/crc_internal.h"
#include "internal/rcc_internal.h"


static uint8_t width_bits(uint8_t width)
{
    switch (width) {        // POLYSIZE: 00 32, 01 16, 10 8, 11 7
        case 16: return 1;
        case 8:  return 2;
        case 7:  return 3;
        default: return 0;
    }
}

void crc_configure(const crc_config_t *cfg)
{
    RCC->AHB1ENR |= BIT(12);                // CRCEN
    (void) RCC->AHB1ENR;
    CRC->POL = cfg->polynomial;
    CRC->INIT = cfg->init;
    CRC->CR = ((uint32_t) width_bits(cfg->width) << 3) | ((uint32_t) cfg->rev_in << 5)
           | (cfg->rev_out ? BIT(7) : 0u);
    crc_reset();
}

void crc_reset(void)
{
    CRC->CR |= BIT(0);                       // RESET: DR = INIT
}

uint32_t crc_value(void)
{
    return CRC->DR;
}

/* Bytes go in as big-endian packed words, then a half-word and/or a byte
 * for the remainder — the same access pattern as ST's HAL, so input
 * reversal (which works on the access size) gives the same results. */
uint32_t crc_feed_bytes(const uint8_t *data, size_t n)
{
    size_t i = 0;
    for (; i + 4u <= n; i += 4u) {
        CRC->DR = ((uint32_t) data[i] << 24) | ((uint32_t) data[i + 1] << 16)
               | ((uint32_t) data[i + 2] << 8) | data[i + 3];
    }
    if (n - i >= 2u) {
        *(volatile uint16_t *) &CRC->DR = (uint16_t) ((data[i] << 8) | data[i + 1]);
        i += 2u;
    }
    if (i < n) *(volatile uint8_t *) &CRC->DR = data[i];
    return CRC->DR;
}

uint32_t crc_feed_halfwords(const uint16_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) *(volatile uint16_t *) &CRC->DR = data[i];
    return CRC->DR;
}

uint32_t crc_feed_words(const uint32_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) CRC->DR = data[i];
    return CRC->DR;
}

/* ----------------------------- software model ----------------------------- */
static uint32_t reflect(uint32_t v, uint8_t bits)
{
    uint32_t r = 0;
    for (uint8_t i = 0; i < bits; i++) if (v & (1u << i)) r |= 1u << (bits - 1u - i);
    return r;
}

static uint32_t rev_input(uint32_t v, uint8_t item_bits, crc_rev_in_t mode)
{
    // reversal granularity: per byte, per half-word or per word, within the item
    uint8_t g = (mode == CRC_REV_IN_BYTE) ? 8 : (mode == CRC_REV_IN_HALFWORD) ? 16 : 32;
    if (mode == CRC_REV_IN_NONE) return v;
    if (g > item_bits) g = item_bits;
    uint32_t out = 0;
    for (uint8_t off = 0; off < item_bits; off += g) {
        uint32_t chunk = (v >> off) & ((g == 32) ? 0xFFFFFFFFu : ((1u << g) - 1u));
        out |= reflect(chunk, g) << off;
    }
    return out;
}

static uint32_t sw_item(const crc_config_t *cfg, uint32_t crc, uint32_t v, uint8_t item_bits)
{
    const uint8_t w = cfg->width;
    const uint32_t mask = (w == 32) ? 0xFFFFFFFFu : ((1u << w) - 1u);
    const uint32_t top = 1u << (w - 1u);
    v = rev_input(v, item_bits, cfg->rev_in);
    // MSB-first: the item's bits enter the CRC register from the top
    for (int b = item_bits - 1; b >= 0; b--) {
        uint32_t in = (v >> b) & 1u;
        uint32_t msb = (crc & top) ? 1u : 0u;
        crc = (crc << 1) & mask;
        if (msb ^ in) crc ^= cfg->polynomial & mask;
    }
    return crc;
}

uint32_t crc_sw(const crc_config_t *cfg, uint32_t crc, const void *data, size_t n, uint8_t item_bits)
{
    if (item_bits == 8) {          // same packing as crc_feed_bytes()
        const uint8_t *b = data;
        const uint8_t w = cfg->width;
        crc &= (w == 32) ? 0xFFFFFFFFu : ((1u << w) - 1u);
        size_t i = 0;
        for (; i + 4u <= n; i += 4u) {
            crc = sw_item(cfg, crc, ((uint32_t) b[i] << 24) | ((uint32_t) b[i + 1] << 16)
                                  | ((uint32_t) b[i + 2] << 8) | b[i + 3], 32);
        }
        if (n - i >= 2u) { crc = sw_item(cfg, crc, (uint32_t) ((b[i] << 8) | b[i + 1]), 16); i += 2u; }
        if (i < n) crc = sw_item(cfg, crc, b[i], 8);
        return cfg->rev_out ? reflect(crc, w) : crc;
    }

    const uint8_t w = cfg->width;
    const uint32_t mask = (w == 32) ? 0xFFFFFFFFu : ((1u << w) - 1u);
    const uint32_t top = 1u << (w - 1u);
    // the output register holds the reversed CRC when rev_out is on: undo for chaining
    crc &= mask;

    for (size_t i = 0; i < n; i++) {
        uint32_t v = (item_bits == 8)  ? ((const uint8_t *) data)[i]
                   : (item_bits == 16) ? ((const uint16_t *) data)[i]
                                       : ((const uint32_t *) data)[i];
        v = rev_input(v, item_bits, cfg->rev_in);
        // MSB-first: the item's bits enter the CRC register from the top
        for (int b = item_bits - 1; b >= 0; b--) {
            uint32_t in = (v >> b) & 1u;
            uint32_t msb = (crc & top) ? 1u : 0u;
            crc = (crc << 1) & mask;
            if (msb ^ in) crc ^= cfg->polynomial & mask;
        }
    }
    return cfg->rev_out ? reflect(crc, w) : crc;
}
