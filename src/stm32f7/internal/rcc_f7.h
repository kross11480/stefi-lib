#pragma once
#include <stdint.h>

/**********************************************************************/
/* STM32F76x/77x RCC — private register map (RM0410 §5.3)             */
/**********************************************************************/
typedef struct {
    volatile uint32_t CR;          /* 0x00 clock control                      */
    volatile uint32_t PLLCFGR;     /* 0x04 main PLL configuration             */
    volatile uint32_t CFGR;        /* 0x08 clock configuration                */
    volatile uint32_t CIR;         /* 0x0C clock interrupt                    */
    volatile uint32_t AHB1RSTR;    /* 0x10 */
    volatile uint32_t AHB2RSTR;    /* 0x14 */
    volatile uint32_t AHB3RSTR;    /* 0x18 */
    uint32_t RESERVED0;            /* 0x1C */
    volatile uint32_t APB1RSTR;    /* 0x20 */
    volatile uint32_t APB2RSTR;    /* 0x24 */
    uint32_t RESERVED1[2];         /* 0x28-0x2C */
    volatile uint32_t AHB1ENR;     /* 0x30 GPIOA..K, DMA1/2 */
    volatile uint32_t AHB2ENR;     /* 0x34 OTGFS, RNG */
    volatile uint32_t AHB3ENR;     /* 0x38 */
    uint32_t RESERVED2;            /* 0x3C */
    volatile uint32_t APB1ENR;     /* 0x40 TIM2..7, USART2/3, PWR */
    volatile uint32_t APB2ENR;     /* 0x44 TIM1/8, SYSCFG, TIM9..11 */
    uint32_t RESERVED3[2];         /* 0x48-0x4C */
    volatile uint32_t AHB1LPENR;   /* 0x50 */
    volatile uint32_t AHB2LPENR;   /* 0x54 */
    volatile uint32_t AHB3LPENR;   /* 0x58 */
    uint32_t RESERVED4;            /* 0x5C */
    volatile uint32_t APB1LPENR;   /* 0x60 */
    volatile uint32_t APB2LPENR;   /* 0x64 */
    uint32_t RESERVED5[2];         /* 0x68-0x6C */
    volatile uint32_t BDCR;        /* 0x70 */
    volatile uint32_t CSR;         /* 0x74 */
    uint32_t RESERVED6[2];         /* 0x78-0x7C */
    volatile uint32_t SSCGR;       /* 0x80 */
    volatile uint32_t PLLI2SCFGR;  /* 0x84 */
    volatile uint32_t PLLSAICFGR;  /* 0x88 */
    volatile uint32_t DCKCFGR1;    /* 0x8C */
    volatile uint32_t DCKCFGR2;    /* 0x90 USARTxSEL, CK48MSEL */
} RCC_F7_t;

#define RCC_F7 ((RCC_F7_t *) 0x40023800u)

/* RCC_CR */
#define RCC_CR_HSION       0u
#define RCC_CR_HSIRDY      1u
#define RCC_CR_HSEON      16u
#define RCC_CR_HSERDY     17u
#define RCC_CR_HSEBYP     18u
#define RCC_CR_PLLON      24u
#define RCC_CR_PLLRDY     25u
#define RCC_CR_PLLSAION   28u
#define RCC_CR_PLLSAIRDY  29u

/* RCC_DCKCFGR2 */
#define RCC_DCKCFGR2_USART3SEL_SHIFT 4u   /* 2 bits: 00 PCLK1, 01 SYSCLK, 10 HSI, 11 LSE */
#define RCC_DCKCFGR2_CK48MSEL       27u   /* 0: PLLQ, 1: PLLSAI P */
