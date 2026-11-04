#pragma once
#include <stdint.h>

/* Low-power timer (L4 RM0351 §32) */
typedef struct {
    volatile uint32_t ISR;       /*!< interrupt and status,          Address offset: 0x00 */
    volatile uint32_t ICR;       /*!< interrupt clear,               Address offset: 0x04 */
    volatile uint32_t IER;       /*!< interrupt enable,              Address offset: 0x08 */
    volatile uint32_t CFGR;      /*!< configuration,                 Address offset: 0x0C */
    volatile uint32_t CR;        /*!< control,                       Address offset: 0x10 */
    volatile uint32_t CMP;       /*!< compare,                       Address offset: 0x14 */
    volatile uint32_t ARR;       /*!< autoreload,                    Address offset: 0x18 */
    volatile uint32_t CNT;       /*!< counter,                       Address offset: 0x1C */
    volatile uint32_t OR;        /*!< option,                        Address offset: 0x20 */
} LPTIM_t;

#define LPTIM1 ((LPTIM_t *) 0x40007C00)
