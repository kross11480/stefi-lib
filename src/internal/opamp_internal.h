#pragma once
#include <stdint.h>

/* Operational amplifiers (L4 RM0351 §24) */
typedef struct {
    volatile uint32_t CSR;       /*!< control/status,                Address offset: 0x00 */
    volatile uint32_t OTR;       /*!< offset trimming, normal mode,  Address offset: 0x04 */
    volatile uint32_t LPOTR;     /*!< offset trimming, low power,    Address offset: 0x08 */
} OPAMP_t;

#define OPAMP1 ((OPAMP_t *) 0x40007800)
#define OPAMP2 ((OPAMP_t *) 0x40007810)
