/* CRC_Example (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 *
 * CRC-32 (default polynomial 0x04C11DB7, init 0xFFFFFFFF) of 114 32-bit
 * words, compared with the CRC computed in software (crc_sw()).
 *   LD2 on: correct CRC, LD2 blinking 1 s: wrong CRC
 * (ST's example uses a fixed data table with a fixed expected CRC; the
 * crc_sw() model reproduces that value — see tests/host.) */
#include "l4_example.h"
#include "libstefi/crc.h"

#define BUFFER_SIZE 114u

static uint32_t data[BUFFER_SIZE];

void main(void)
{
    board_init();
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) data[i] = 0x00001021u * (i + 1u) ^ (i << 20);

    const crc_config_t cfg = CRC_CONFIG_DEFAULT;
    crc_configure(&cfg);
    uint32_t crc = crc_feed_words(data, BUFFER_SIZE);
    if (crc != crc_sw(&cfg, cfg.init, data, BUFFER_SIZE, 32)) example_error();
    example_success();
}
