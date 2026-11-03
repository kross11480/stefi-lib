/**********************************************************************/
/* STM32F7 RCC clock gating — same API as src/peripheral.c (L4/G4)    */
/**********************************************************************/
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/rcc_f7.h"

typedef struct {
    volatile uint32_t *rcc_reg;
    uint8_t enable_bit;
} peripheral_dev_t;

static const peripheral_dev_t peripheral_tim_dev[PERIPHERAL_TIM_COUNT] = {
    [PERIPHERAL_TIM1]  = {&RCC_F7->APB2ENR, 0},
    [PERIPHERAL_TIM2]  = {&RCC_F7->APB1ENR, 0},
    [PERIPHERAL_TIM3]  = {&RCC_F7->APB1ENR, 1},
    [PERIPHERAL_TIM4]  = {&RCC_F7->APB1ENR, 2},
    [PERIPHERAL_TIM5]  = {&RCC_F7->APB1ENR, 3},
    [PERIPHERAL_TIM6]  = {&RCC_F7->APB1ENR, 4},
    [PERIPHERAL_TIM7]  = {&RCC_F7->APB1ENR, 5},
    [PERIPHERAL_TIM8]  = {&RCC_F7->APB2ENR, 1},
    [PERIPHERAL_TIM15] = {&RCC_F7->APB2ENR, 16},  /* TIM9  */
    [PERIPHERAL_TIM16] = {&RCC_F7->APB2ENR, 17},  /* TIM10 */
    [PERIPHERAL_TIM17] = {&RCC_F7->APB2ENR, 18},  /* TIM11 */
};

static const peripheral_dev_t peripheral_uart_dev[PERIPHERAL_UART_COUNT] = {
    [PERIPHERAL_UART1] = {&RCC_F7->APB2ENR, 4},
    [PERIPHERAL_UART2] = {&RCC_F7->APB1ENR, 17},
    [PERIPHERAL_UART3] = {&RCC_F7->APB1ENR, 18},
};

void peripheral_gpio_enable(peripheral_gpio_t id)
{
    RCC_F7->AHB1ENR |= BIT(id);   /* GPIOAEN..GPIOKEN = bits 0..10 */
    (void) RCC_F7->AHB1ENR;       /* RM0410: wait 2 cycles before access */
}

void peripheral_tim_enable(peripheral_tim_t id)
{
    // starting from 1 the timer id and not zero (same as L4/G4)
    *peripheral_tim_dev[id - 1].rcc_reg |= BIT(peripheral_tim_dev[id - 1].enable_bit);
    (void) *peripheral_tim_dev[id - 1].rcc_reg;
}

void peripheral_uart_enable(peripheral_uart_t id)
{
    *peripheral_uart_dev[id].rcc_reg |= BIT(peripheral_uart_dev[id].enable_bit);
    (void) *peripheral_uart_dev[id].rcc_reg;
}

void peripheral_dma_enable(peripheral_dma_t id)
{
    RCC_F7->AHB1ENR |= (1u << (21u + id));  /* DMA1EN = 21, DMA2EN = 22 */
    (void) RCC_F7->AHB1ENR;
}

void peripheral_exti_enable()
{
    RCC_F7->APB2ENR |= BIT(14);       /* SYSCFGEN */
    (void) RCC_F7->APB2ENR;
}

void peripheral_usb_otg_fs_enable(void)
{
    RCC_F7->AHB2ENR |= BIT(7);        /* OTGFSEN */
    (void) RCC_F7->AHB2ENR;
}

/* Not ported to the F7 yet (the HAL drivers behind them are L4/G4-only). */
void peripheral_i2c_enable(peripheral_i2c_t id) { (void) id; }
void peripheral_spi_enable(peripheral_spi_t id) { (void) id; }
void peripheral_adc_enable(peripheral_adc_t id) { (void) id; }
void peripheral_rng_enable() { RCC_F7->AHB2ENR |= BIT(6); }
