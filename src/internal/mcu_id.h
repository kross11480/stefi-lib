#pragma once
#include <stdbool.h>
#include <stdint.h>

/*
 * L4 vs G4 at run time. The two share one build (family STM32L4G4) and
 * almost one register map; where they differ (DMA request routing, USB)
 * the drivers ask the debug unit's device ID instead of needing a
 * compile-time switch. DBGMCU_IDCODE DEV_ID: L476/486 = 0x415,
 * G431/441 = 0x468, G47x/48x = 0x469, G491/4A1 = 0x479.
 */
#define DBGMCU_IDCODE (*(volatile const uint32_t *) 0xE0042000u)

static inline uint16_t mcu_dev_id(void)
{
    return (uint16_t) (DBGMCU_IDCODE & 0xFFFu);
}

static inline bool mcu_is_g4(void)
{
    uint16_t id = mcu_dev_id();
    return id == 0x468u || id == 0x469u || id == 0x479u;
}

/* DMA channels per controller: 6 on the G431/G491, 8 on the G47x, 7 on L4. */
static inline uint8_t mcu_dma_channels(void)
{
    uint16_t id = mcu_dev_id();
    if (id == 0x468u || id == 0x479u) return 6u;
    if (id == 0x469u) return 8u;
    return 7u;
}
