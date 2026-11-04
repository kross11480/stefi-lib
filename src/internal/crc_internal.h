#pragma once
#include <stdint.h>

/* CRC calculation unit (L4 RM0351 §14 / G4 RM0440 §16) */
typedef struct {
    volatile uint32_t DR;        /*!< data register,                 Address offset: 0x00 */
    volatile uint32_t IDR;       /*!< independent data register,     Address offset: 0x04 */
    volatile uint32_t CR;        /*!< control register,              Address offset: 0x08 */
    uint32_t RESERVED;           /*!< Reserved,                      Address offset: 0x0C */
    volatile uint32_t INIT;      /*!< initial CRC value,             Address offset: 0x10 */
    volatile uint32_t POL;       /*!< polynomial,                    Address offset: 0x14 */
} CRC_t;

#define CRC ((CRC_t *) 0x40023000)
