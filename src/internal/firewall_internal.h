#pragma once
#include <stdint.h>

/* Firewall (L4 RM0351 §4) */
typedef struct {
    volatile uint32_t CSSA;      /*!< code segment start address,    Address offset: 0x00 */
    volatile uint32_t CSL;       /*!< code segment length,           Address offset: 0x04 */
    volatile uint32_t NVDSSA;    /*!< non-volatile data start,       Address offset: 0x08 */
    volatile uint32_t NVDSL;     /*!< non-volatile data length,      Address offset: 0x0C */
    volatile uint32_t VDSSA;     /*!< volatile data start,           Address offset: 0x10 */
    volatile uint32_t VDSL;      /*!< volatile data length,          Address offset: 0x14 */
    uint32_t RESERVED[2];        /*!< Reserved,                      Address offset: 0x18 */
    volatile uint32_t CR;        /*!< configuration,                 Address offset: 0x20 */
} FIREWALL_t;

#define FIREWALL ((FIREWALL_t *) 0x40011C00)
