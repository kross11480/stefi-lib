#pragma once
#include <stdint.h>

/* Independent watchdog (L4/G4) */
typedef struct {
    volatile uint32_t KR;        /*!< key,                           Address offset: 0x00 */
    volatile uint32_t PR;        /*!< prescaler,                     Address offset: 0x04 */
    volatile uint32_t RLR;       /*!< reload,                        Address offset: 0x08 */
    volatile uint32_t SR;        /*!< status,                        Address offset: 0x0C */
    volatile uint32_t WINR;      /*!< window,                        Address offset: 0x10 */
} IWDG_t;

#define IWDG ((IWDG_t *) 0x40003000)
