#pragma once

/**********************************************************************/
/* Interrupts Hardware Abstraction layer */
/**********************************************************************/

#include <stdint.h>

//Datatypes
typedef void (*callbackfn_t)(void *aux_data);

enum _nvic_interrupt_sources {
    INTERRUPT_SOURCE_NONE = -1,
    INTERRUPT_SOURCE_DMA1_CH1 = 11,
    INTERRUPT_SOURCE_DMA1_CH2 = 12,
    INTERRUPT_SOURCE_DMA1_CH3 = 13,
    INTERRUPT_SOURCE_DMA1_CH4 = 14,
    INTERRUPT_SOURCE_DMA1_CH5 = 15,
    INTERRUPT_SOURCE_DMA1_CH6 = 16,
    INTERRUPT_SOURCE_DMA1_CH7 = 17,
    INTERRUPT_SOURCE_EXTI0 = 6,
    INTERRUPT_SOURCE_EXTI1 = 7,
    INTERRUPT_SOURCE_EXTI2 = 8,
    INTERRUPT_SOURCE_EXTI3 = 9,
    INTERRUPT_SOURCE_EXTI4 = 10,
    INTERRUPT_SOURCE_EXTI5_9 = 23,
    INTERRUPT_SOURCE_EXTI10_15 = 40,
    INTERRUPT_SOURCE_TIM2 = 28,
    INTERRUPT_SOURCE_TIM3 = 29,
    INTERRUPT_SOURCE_TIM4 = 30,
    INTERRUPT_SOURCE_TIM6 = 54,
    INTERRUPT_SOURCE_TIM7 = 55,
    INTERRUPT_SOURCE_TIM8 = 44,
    INTERRUPT_SOURCE_TIM15 = 24,
    INTERRUPT_SOURCE_TIM1_TIM16 = 25,
    INTERRUPT_SOURCE_TIM17 = 26,
#if defined(STEFI_MCU_STM32F7)
    /* STM32F76x/77x only (RM0410 vector table). The EXTI and TIM2/3/4/6/7/8
     * lines above have the same numbers on the F7. On the F7, DMA1_CH1..7
     * above are DMA1 *streams* 0..6. */
    INTERRUPT_SOURCE_TIM1_UP = 25,           /* TIM1_UP_TIM10 */
    INTERRUPT_SOURCE_USART3 = 39,
    INTERRUPT_SOURCE_DMA1_STREAM0 = 11,      /* streams 0..6 contiguous */
    INTERRUPT_SOURCE_DMA1_STREAM7 = 47,
    INTERRUPT_SOURCE_DMA2_STREAM0 = 56,      /* streams 0..4 contiguous */
    INTERRUPT_SOURCE_DMA2_STREAM5 = 68,      /* streams 5..7 contiguous */
    INTERRUPT_SOURCE_OTG_FS = 67,
#endif
};

typedef enum _nvic_interrupt_sources nvic_source_t;

/* Interrupt initialization functions*/

/**
 * @brief Enable interrupt module
 *
 */
void interrupts_init(void);

/**
 * @brief Enable interrupt handlers
 *
 */
void interrupts_global_enable(void);

/**
 * @brief Disable interrupt handlers
 *
 */
void interrupts_global_disable(void);

/**
 * @brief Enable interrupt #source
 *
 */
void interrupts_enable_source(nvic_source_t source);

/**
 * @brief Disable interrupt #source
 *
 */
void interrupts_disable_source(nvic_source_t source);

/**
 * @brief Set the NVIC priority of #source (0 = highest .. 15 = lowest).
 *
 * STM32 implements 4 priority bits; all sources reset to 0. A lower number
 * preempts a higher one.
 */
void interrupts_set_priority(nvic_source_t source, uint8_t priority);

/**
 * @brief Register a callback for the given interrupt source
 *
 */
void interrupts_register_handler(nvic_source_t source, callbackfn_t fn);
