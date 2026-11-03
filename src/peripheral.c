
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "../src/internal/rcc_internal.h"

typedef struct {
    volatile uint32_t* rcc_reg;
    uint8_t enable_bit;
} peripheral_dev_t;

static const peripheral_dev_t peripheral_gpio_dev[PERIPHERAL_GPIO_COUNT] = {
    [PERIPHERAL_GPIOA] = {&RCC->AHB2ENR, 0},
    [PERIPHERAL_GPIOB] = {&RCC->AHB2ENR, 1},
    [PERIPHERAL_GPIOC] = {&RCC->AHB2ENR, 2},
    [PERIPHERAL_GPIOD] = {&RCC->AHB2ENR, 3},
    [PERIPHERAL_GPIOE] = {&RCC->AHB2ENR, 4},
    [PERIPHERAL_GPIOF] = {&RCC->AHB2ENR, 5},
    [PERIPHERAL_GPIOG] = {&RCC->AHB2ENR, 6},
    [PERIPHERAL_GPIOH] = {&RCC->AHB2ENR, 7},
};

static const peripheral_dev_t peripheral_tim_dev[PERIPHERAL_TIM_COUNT] = {
    [PERIPHERAL_TIM1] = {&RCC->APB2ENR, 11},
    [PERIPHERAL_TIM2] = {&RCC->APB1ENR1, 0},
    [PERIPHERAL_TIM3] = {&RCC->APB1ENR1, 1},
    [PERIPHERAL_TIM4] = {&RCC->APB1ENR1, 2},
    [PERIPHERAL_TIM5] = {&RCC->APB1ENR1, 3},
    [PERIPHERAL_TIM6] = {&RCC->APB1ENR1, 4},
    [PERIPHERAL_TIM7] = {&RCC->APB1ENR1, 5},
    [PERIPHERAL_TIM8] = {&RCC->APB2ENR, 13},
    [PERIPHERAL_TIM15] = {&RCC->APB2ENR, 16},
    [PERIPHERAL_TIM16] = {&RCC->APB2ENR, 17},
    [PERIPHERAL_TIM17] = {&RCC->APB2ENR, 18},
};

static const peripheral_dev_t peripheral_uart_dev[PERIPHERAL_UART_COUNT] = {
    [PERIPHERAL_UART1] = {&RCC->APB2ENR, 14},
    [PERIPHERAL_UART2] = {&RCC->APB1ENR1, 17},
    [PERIPHERAL_UART3] = {&RCC->APB1ENR1, 18},
};

static const peripheral_dev_t peripheral_i2c_dev[PERIPHERAL_I2C_COUNT] = {
    [PERIPHERAL_I2C1] = {&RCC->APB1ENR1, 21},
    [PERIPHERAL_I2C2] = {&RCC->APB1ENR1, 22},
    [PERIPHERAL_I2C3] = {&RCC->APB1ENR1, 23},
};

static const peripheral_dev_t peripheral_spi_dev[PERIPHERAL_SPI_COUNT] = {
    [PERIPHERAL_SPI1] = {&RCC->APB2ENR, 12},
    [PERIPHERAL_SPI2] = {&RCC->APB1ENR1, 14},
    [PERIPHERAL_SPI3] = {&RCC->APB1ENR1, 15},
};

static const peripheral_dev_t peripheral_adc_dev[PERIPHERAL_ADC_COUNT] = {
    [PERIPHERAL_ADC1] = {&RCC->AHB2ENR, 13},
    [PERIPHERAL_ADC2] = {&RCC->AHB2ENR, 13},
    [PERIPHERAL_ADC3] = {&RCC->AHB2ENR, 13},
};

static const peripheral_dev_t peripheral_dma_dev[PERIPHERAL_DMA_COUNT] = {
    [PERIPHERAL_DMA1] = {&RCC->AHB1ENR, 0},  // DMA1EN
    [PERIPHERAL_DMA2] = {&RCC->AHB1ENR, 1},  // DMA2EN
};

void peripheral_gpio_enable(peripheral_gpio_t id)
{
    uint8_t pos = peripheral_gpio_dev[id].enable_bit;
    *peripheral_gpio_dev[id].rcc_reg |= BIT(pos);
}

void peripheral_tim_enable(peripheral_tim_t id)
{
    // starting from 1 the timer id and not zero
    uint8_t pos = peripheral_tim_dev[id-1].enable_bit;
    *peripheral_tim_dev[id-1].rcc_reg |= BIT(pos);
}

//change later to id-1
void peripheral_uart_enable(peripheral_uart_t id)
{
    uint8_t pos = peripheral_uart_dev[id].enable_bit;
    *peripheral_uart_dev[id].rcc_reg |= BIT(pos);
}

void peripheral_exti_enable() {
    RCC->APB2ENR |= BIT(0);//SYSCFG Enable
}

void peripheral_i2c_enable(peripheral_i2c_t id)
{
    uint8_t pos = peripheral_i2c_dev[id-1].enable_bit;
    *peripheral_i2c_dev[id].rcc_reg |= BIT(pos);
}

void peripheral_rng_enable()
{
    RCC->AHB2ENR |= BIT(18);
    //For L476, better set pll to 48 MHZ for portability
    RCC->CCIPR |= (0x3 << 26);
}

void peripheral_spi_enable(peripheral_spi_t id)
{
    uint8_t pos = peripheral_spi_dev[id].enable_bit;
    *peripheral_spi_dev[id].rcc_reg |= BIT(pos);
}

void peripheral_adc_enable(peripheral_adc_t id)
{
    uint8_t pos = peripheral_adc_dev[id].enable_bit;
    *peripheral_adc_dev[id].rcc_reg |= BIT(pos);
}

void peripheral_dma_enable(peripheral_dma_t id)
{
    uint8_t pos = peripheral_dma_dev[id].enable_bit;
    *peripheral_dma_dev[id].rcc_reg |= BIT(pos);
    // DMAMUX1EN (AHB1ENR bit 2): request routing on G4. Reserved/no-op on L476
    // (which routes via DMA_CSELR instead), so harmless to set on both.
    RCC->AHB1ENR |= BIT(2);
}