/* CRC_Data_Reversing_16bit_CRC (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * 16-bit CRC-CCITT (polynomial 0x1021), init 0x5ABE, byte input, input
 * bit reversal per word and output reversal: 9 bytes -> 0x5043.
 * Check: the bit-reversed input buffer without any reversal must give the
 * bit-reversed CRC (0x5043 reflected = 0xC20A).
 * Same test vectors and expected value as ST's example.
 *   LD2 on: both correct, LD2 blinking 1 s: wrong CRC */
#include "l4_example.h"
#include "libstefi/crc.h"

static const uint8_t data[9]       = {0x4D, 0x3C, 0x2B, 0x1A, 0xBE, 0x71, 0xC9, 0x8A, 0x5E};
static const uint8_t data_check[9] = {0x58, 0xD4, 0x3C, 0xB2, 0x51, 0x93, 0x8E, 0x7D, 0x7A};
#define EXPECTED 0x5043u

void main(void)
{
    board_init();

    crc_config_t cfg = {.polynomial = 0x1021, .width = 16, .init = 0x5ABE,
                        .rev_in = CRC_REV_IN_WORD, .rev_out = true};
    crc_configure(&cfg);
    if (crc_feed_bytes(data, sizeof data) != EXPECTED) example_error();

    cfg.rev_in = CRC_REV_IN_NONE;
    cfg.rev_out = false;
    crc_configure(&cfg);
    uint32_t want = 0;
    for (int i = 0; i < 16; i++) if (EXPECTED & (1u << i)) want |= 1u << (15 - i);
    if (crc_feed_bytes(data_check, sizeof data_check) != want) example_error();
    example_success();
}
