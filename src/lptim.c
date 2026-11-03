/**********************************************************************/
/* LPTIM1 (STM32L4, RM0351 §32)                                       */
/**********************************************************************/
#include "libstefi/lptim.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/gpio_internal.h"
#include "internal/rcc_internal.h"

#include <stddef.h>

#define LPTIM_REG(off) (*(volatile uint32_t *) (0x40007C00u + (off)))
#define LPTIM_ISR  LPTIM_REG(0x00)
#define LPTIM_ICR  LPTIM_REG(0x04)
#define LPTIM_IER  LPTIM_REG(0x08)
#define LPTIM_CFGR LPTIM_REG(0x0C)
#define LPTIM_CR   LPTIM_REG(0x10)
#define LPTIM_CMP  LPTIM_REG(0x14)
#define LPTIM_ARR  LPTIM_REG(0x18)
#define LPTIM_CNT  LPTIM_REG(0x1C)

#define CR_ENABLE  BIT(0)
#define CR_CNTSTRT BIT(2)
#define ISR_ARRM   BIT(1)
#define ISR_CMPOK  BIT(3)
#define ISR_ARROK  BIT(4)
#define EXTI_LINE_LPTIM1 0u        /* EXTI line 32 = bit 0 of the second bank */

static struct { callbackfn_t fn; void *aux; } arr_cb;

static void pin_af1(gpio_id_t pin)
{
    peripheral_gpio_enable(gpio_get_port_from_portpin(pin));
    gpio_set_alternate_function(pin, AF1);
    gpio_set_output_speed(pin, LOW_S);
    gpio_set_mode(pin, MODER_AF);
}

void lptim_init(lptim_kernel_clock_t clk, lptim_count_t count, uint8_t presc_log2)
{
    if (clk == LPTIM_CLK_LSI) {
        #define RCC_CSR_R (*(volatile uint32_t *) 0x40021094u)
        RCC_CSR_R |= BIT(0);
        while (!(RCC_CSR_R & BIT(1)));
    }
    RCC->CCIPR = (RCC->CCIPR & ~(0x3u << 18)) | ((uint32_t) clk << 18);   // LPTIM1SEL
    RCC->APB1ENR1 |= (1u << 31);                     // LPTIM1EN
    (void) RCC->APB1ENR1;

    LPTIM_CR = 0;
    uint32_t cfgr = (uint32_t) (presc_log2 & 7u) << 9;
    if (count != LPTIM_COUNT_INTERNAL) {
        pin_af1(B5);                                 // IN1
        cfgr |= BIT(23);                             // COUNTMODE: external IN1
        if (count == LPTIM_COUNT_IN1_CLOCK) cfgr |= BIT(0);   // CKSEL: IN1 clocks the counter
    }
    LPTIM_CFGR = cfgr;
}

static void enable_and_load(uint16_t arr, uint16_t cmp)
{
    LPTIM_CR = CR_ENABLE;
    LPTIM_ICR = ISR_ARROK | ISR_CMPOK;
    LPTIM_ARR = arr;                                 // ARR/CMP only writable while enabled
    while (!(LPTIM_ISR & ISR_ARROK));
    LPTIM_CMP = cmp;
    while (!(LPTIM_ISR & ISR_CMPOK));
}

void lptim_pwm_start(uint16_t arr, uint16_t cmp)
{
    pin_af1(B2);                                     // OUT
    enable_and_load(arr, cmp);
    LPTIM_CR |= CR_CNTSTRT;
}

void lptim_counter_start(uint16_t arr, callbackfn_t on_arr_match, void *aux)
{
    arr_cb.fn = on_arr_match;
    arr_cb.aux = aux;
    LPTIM_IER = ISR_ARRM;                            // only while disabled
    enable_and_load(arr, 0);
    // interrupt + wakeup from Stop through EXTI line 32
    EXTI->IMR2 |= BIT(EXTI_LINE_LPTIM1);
    interrupts_enable_source((nvic_source_t) 65);    // LPTIM1
    LPTIM_CR |= CR_CNTSTRT;
}

void lptim_stop(void)
{
    LPTIM_CR = 0;
}

uint16_t lptim_count(void)
{
    // asynchronous counter: read until two consecutive reads agree
    uint32_t a, b;
    do { a = LPTIM_CNT; b = LPTIM_CNT; } while (a != b);
    return (uint16_t) a;
}

void LPTIM1_IRQHandler(void)
{
    uint32_t isr = LPTIM_ISR;
    LPTIM_ICR = isr;
    if ((isr & ISR_ARRM) && arr_cb.fn) arr_cb.fn(arr_cb.aux);
}
