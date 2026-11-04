#pragma once
#include <stdint.h>

/* Filter math accelerator (G4 RM0440 §18) */
typedef struct {
    volatile uint32_t X1BUFCFG;  /*!< X1 buffer configuration,       Address offset: 0x00 */
    volatile uint32_t X2BUFCFG;  /*!< X2 buffer configuration,       Address offset: 0x04 */
    volatile uint32_t YBUFCFG;   /*!< Y buffer configuration,        Address offset: 0x08 */
    volatile uint32_t PARAM;     /*!< parameters,                    Address offset: 0x0C */
    volatile uint32_t CR;        /*!< control,                       Address offset: 0x10 */
    volatile uint32_t SR;        /*!< status,                        Address offset: 0x14 */
    volatile uint32_t WDATA;     /*!< write data,                    Address offset: 0x18 */
    volatile uint32_t RDATA;     /*!< read data,                     Address offset: 0x1C */
} FMAC_t;

#define FMAC ((FMAC_t *) 0x40021400)
