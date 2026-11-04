#pragma once
#include <stdint.h>

/* CORDIC co-processor (G4 RM0440 §17) */
typedef struct {
    volatile uint32_t CSR;       /*!< control and status,            Address offset: 0x00 */
    volatile uint32_t WDATA;     /*!< argument,                      Address offset: 0x04 */
    volatile uint32_t RDATA;     /*!< result,                        Address offset: 0x08 */
} CORDIC_t;

#define CORDIC ((CORDIC_t *) 0x40020C00)
