#pragma once
#include "stdint.h"

typedef struct
{
  volatile uint32_t CR1;         /*!< TIM control register 1,                   Address offset: 0x00 */
  volatile uint32_t CR2;         /*!< TIM control register 2,                   Address offset: 0x04 */
  volatile uint32_t SMCR;        /*!< TIM slave mode control register,          Address offset: 0x08 */
  volatile uint32_t DIER;        /*!< TIM DMA/interrupt enable register,        Address offset: 0x0C */
  volatile uint32_t SR;          /*!< TIM status register,                      Address offset: 0x10 */
  volatile uint32_t EGR;         /*!< TIM event generation register,            Address offset: 0x14 */
  volatile uint32_t CCMR1;       /*!< TIM capture/compare mode register 1,      Address offset: 0x18 */
  volatile uint32_t CCMR2;       /*!< TIM capture/compare mode register 2,      Address offset: 0x1C */
  volatile uint32_t CCER;        /*!< TIM capture/compare enable register,      Address offset: 0x20 */
  volatile uint32_t CNT;         /*!< TIM counter register,                     Address offset: 0x24 */
  volatile uint32_t PSC;         /*!< TIM prescaler,                            Address offset: 0x28 */
  volatile uint32_t ARR;         /*!< TIM auto-reload register,                 Address offset: 0x2C */
  volatile uint32_t RCR;         /*!< TIM repetition counter register,          Address offset: 0x30 */
  volatile uint32_t CCR1;        /*!< TIM capture/compare register 1,           Address offset: 0x34 */
  volatile uint32_t CCR2;        /*!< TIM capture/compare register 2,           Address offset: 0x38 */
  volatile uint32_t CCR3;        /*!< TIM capture/compare register 3,           Address offset: 0x3C */
  volatile uint32_t CCR4;        /*!< TIM capture/compare register 4,           Address offset: 0x40 */
  volatile uint32_t BDTR;        /*!< TIM break and dead-time register,         Address offset: 0x44 */
  volatile uint32_t CCR5;        /*!< TIM capture/compare register 5,           Address offset: 0x48 */
  volatile uint32_t CCR6;        /*!< TIM capture/compare register 6,           Address offset: 0x4C */
  volatile uint32_t CCMR3;       /*!< TIM capture/compare mode register 3,      Address offset: 0x50 */
  volatile uint32_t DTR2;        /*!< TIM deadtime register 2,                  Address offset: 0x54 */
  volatile uint32_t ECR;         /*!< TIM encoder control register,             Address offset: 0x58 */
  volatile uint32_t TISEL;       /*!< TIM Input Selection register,             Address offset: 0x5C */
  volatile uint32_t AF1;         /*!< TIM alternate function option register 1, Address offset: 0x60 */
  volatile uint32_t AF2;         /*!< TIM alternate function option register 2, Address offset: 0x64 */
  volatile uint32_t OR ;         /*!< TIM option register,                      Address offset: 0x68 */
  volatile uint32_t RESERVED0[220];/*!< Reserved,                               Address offset: 0x6C */
  volatile uint32_t DCR;         /*!< TIM DMA control register,                 Address offset: 0x3DC */
  volatile uint32_t DMAR;        /*!< TIM DMA address for full transfer,        Address offset: 0x3E0 */
} TIM_t;

#define NUM_TIMERINTERRUPTS 19

/*
 * Register layout above is the G4/L4 one. The F7 timers share it up to BDTR
 * (0x44), which is everything timer.c touches; the F7-only fields beyond it
 * (CCMR3/CCR5/CCR6 at different offsets) must not be used on the F7.
 *
 * On the F7, TIMER15/16/17 alias TIM9/10/11: same addresses, same RCC enable
 * bits and same NVIC lines (TIM1_BRK_TIM9, TIM1_UP_TIM10, TIM1_TRG_COM_TIM11).
 */
#if defined(STEFI_MCU_STM32F7)
#define TIM1_BASE ((TIM_t *) 0x40010000)
#define TIM8_BASE ((TIM_t *) 0x40010400)
#else
#define TIM1_BASE ((TIM_t *) 0x40012C00)
#define TIM8_BASE ((TIM_t *) 0x40013400)
#endif
#define TIM2_BASE ((TIM_t *) 0x40000000)
#define TIM3_BASE ((TIM_t *) 0x40000400)
#define TIM4_BASE ((TIM_t *) 0x40000800)
#define TIM5_BASE ((TIM_t *) 0x40000C00)
#define TIM6_BASE ((TIM_t *) 0x40001000)
#define TIM7_BASE ((TIM_t *) 0x40001400)

#define TIM15_BASE ((TIM_t *) 0x40014000)
#define TIM16_BASE ((TIM_t *) 0x40014400)
#define TIM17_BASE ((TIM_t *) 0x40014800)

//For compatibility, low level test code
#define TIM1 TIM1_BASE
#define TIM2 TIM2_BASE
#define TIM3 TIM3_BASE
#define TIM4 TIM4_BASE
#define TIM5 TIM5_BASE
#define TIM6 TIM6_BASE
#define TIM7 TIM7_BASE
#define TIM8 TIM8_BASE
#define TIM15 TIM15_BASE
#define TIM16 TIM16_BASE
#define TIM17 TIM17_BASE

/* L4 only: TIM16 option register 1 (TI1_RMP: CH1 input from LSI/LSE/RTC),
 * at an offset where the G4 layout above has CCMR3. */
#define TIM16_L4_OR1 (*(volatile uint32_t *) (0x40014400 + 0x50))
