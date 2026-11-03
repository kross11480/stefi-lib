#pragma once
#include <stdint.h>

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t ICSCR;       /*!< RCC internal clock sources calibration register,                         Address offset: 0x04 */
    volatile uint32_t CFGR;        /*!< RCC clock configuration register,                                        Address offset: 0x08 */
    volatile uint32_t PLLCFGR;     /*!< RCC system PLL configuration register,                                   Address offset: 0x0C */
    volatile uint32_t REGS_NOTNEEDEDNOW[14]; // 0x10 .. 0x44
    volatile uint32_t AHB1ENR;  // AHB1 peripheral clock enable register      Address offset: 0x48
    volatile uint32_t AHB2ENR;  // AHB2 peripheral clock enable register      Address offset: 0x4C
    volatile uint32_t AHB3ENR;  // AHB3 peripheral clock enable register
    volatile uint32_t RESERVED;
    volatile uint32_t APB1ENR1;  // APB1 peripheral clock enable register
    volatile uint32_t APB1ENR2;
    volatile uint32_t APB2ENR;
    uint32_t      RESERVED4;   /*!< Reserved,                                                                Address offset: 0x64 */
    volatile uint32_t AHB1SMENR;   /*!< RCC AHB1 peripheral clocks enable in sleep and stop modes register,      Address offset: 0x68 */
    volatile uint32_t AHB2SMENR;   /*!< RCC AHB2 peripheral clocks enable in sleep and stop modes register,      Address offset: 0x6C */
    volatile uint32_t AHB3SMENR;   /*!< RCC AHB3 peripheral clocks enable in sleep and stop modes register,      Address offset: 0x70 */
    uint32_t      RESERVED5;   /*!< Reserved,                                                                Address offset: 0x74 */
    volatile uint32_t APB1SMENR1;  /*!< RCC APB1 peripheral clocks enable in sleep mode and stop modes register 1, Address offset: 0x78 */
    volatile uint32_t APB1SMENR2;  /*!< RCC APB1 peripheral clocks enable in sleep mode and stop modes register 2, Address offset: 0x7C */
    volatile uint32_t APB2SMENR;   /*!< RCC APB2 peripheral clocks enable in sleep mode and stop modes register, Address offset: 0x80 */
    uint32_t      RESERVED6;   /*!< Reserved,                                                                Address offset: 0x84 */
    volatile uint32_t CCIPR;       /*!< peripherals independent clock config (CLK48SEL [27:26]), 0x88 */
    uint32_t      RESERVED7;   /*!< Reserved,                                                                Address offset: 0x8C */
    volatile uint32_t BDCR;        /*!< backup domain control (LSEON [0], LSERDY [1]),                         0x90 */
} RCC_typeDef;

#define RCC ((RCC_typeDef *) 0x40021000) //Operator precedence remove ()