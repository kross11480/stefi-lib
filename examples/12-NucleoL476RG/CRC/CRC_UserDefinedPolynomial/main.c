/* CRC_UserDefinedPolynomial (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * 8-bit CRC with the user polynomial 0x9B (X^8 + X^7 + X^4 + X^3 + X + 1),
 * default init, of one 32-bit word 0x1234 -> 0xEF (ST's test vector).
 *   LD2 on: correct, LD2 blinking 1 s: wrong */
#include "l4_example.h"
#include "libstefi/crc.h"

static const uint32_t data = 0x1234;

void main(void)
{
    board_init();
    const crc_config_t cfg = {.polynomial = 0x9B, .width = 8, .init = 0xFFFFFFFFu};
    crc_configure(&cfg);
    if (crc_feed_words(&data, 1) != 0xEF) example_error();
    example_success();
}
