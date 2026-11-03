#pragma once
#include <stdint.h>

/**********************************************************************/
/* STM32F7 PWR / FLASH interface / SCB — private register maps        */
/**********************************************************************/
typedef struct {
    volatile uint32_t CR1;    /* 0x00 VOS[15:14], ODEN[16], ODSWEN[17] */
    volatile uint32_t CSR1;   /* 0x04 VOSRDY[14], ODRDY[16], ODSWRDY[17] */
} PWR_F7_t;
#define PWR_F7 ((PWR_F7_t *) 0x40007000u)

typedef struct {
    volatile uint32_t ACR;    /* 0x00 LATENCY[3:0], PRFTEN[8], ARTEN[9] */
} FLASH_F7_t;
#define FLASH_F7 ((FLASH_F7_t *) 0x40023C00u)
