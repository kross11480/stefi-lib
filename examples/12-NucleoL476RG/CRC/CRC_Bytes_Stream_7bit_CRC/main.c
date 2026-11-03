/* CRC_Bytes_Stream_7bit_CRC (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * 7-bit CRC, polynomial 0x65 (X^7 + X^6 + X^5 + X^2 + 1, as in the Train
 * Communication Network IEC 60870-5), default init, byte input:
 *   1. CRC of 5 bytes                         -> 0x57
 *   2. continued (no reset) over 17 more bytes -> 0x6E
 *   3. continued over 1 more byte              -> 0x4B
 *   4. new CRC (reset) of 2 bytes              -> 0x26
 * Same test vectors and expected values as ST's example.
 *   LD2 on: all four correct, LD2 blinking 1 s: a CRC is wrong */
#include "l4_example.h"
#include "libstefi/crc.h"

static const uint8_t test5[5]   = {0x12, 0x34, 0xBA, 0x71, 0xAD};
static const uint8_t test17[17] = {0x12, 0x34, 0xBA, 0x71, 0xAD, 0x11, 0x56, 0xDC, 0x88,
                                   0x1B, 0xEE, 0x4D, 0x82, 0x93, 0xA6, 0x7F, 0xC3};
static const uint8_t test1[1]   = {0x19};
static const uint8_t test2[2]   = {0xAB, 0xCD};

void main(void)
{
    board_init();
    const crc_config_t cfg = {.polynomial = 0x65, .width = 7, .init = 0xFFFFFFFFu};
    crc_configure(&cfg);

    if (crc_feed_bytes(test5, sizeof test5) != 0x57) example_error();
    if (crc_feed_bytes(test17, sizeof test17) != 0x6E) example_error();
    if (crc_feed_bytes(test1, sizeof test1) != 0x4B) example_error();
    crc_reset();
    if (crc_feed_bytes(test2, sizeof test2) != 0x26) example_error();
    example_success();
}
