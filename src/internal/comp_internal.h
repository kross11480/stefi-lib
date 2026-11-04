#pragma once
#include <stdint.h>

/* Comparators (L4 RM0351 §22): one control/status register each */
typedef struct {
    volatile uint32_t CSR;       /*!< COMPx control and status,      Address offset: 0x00 */
} COMP_t;

#define COMP1 ((COMP_t *) 0x40010200)
#define COMP2 ((COMP_t *) 0x40010204)
