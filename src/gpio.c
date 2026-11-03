#include "libstefi/gpio.h"
#include "../src/internal/gpio_internal.h"
#include <stdint.h>
#include <libstefi/peripheral.h>
#include <libstefi/util.h>

static const enum _nvic_interrupt_sources extipin_to_irq[16] = {
    INTERRUPT_SOURCE_EXTI0,
    INTERRUPT_SOURCE_EXTI1,
    INTERRUPT_SOURCE_EXTI2,
    INTERRUPT_SOURCE_EXTI3,
    INTERRUPT_SOURCE_EXTI4,
    INTERRUPT_SOURCE_EXTI5_9,
    INTERRUPT_SOURCE_EXTI5_9,
    INTERRUPT_SOURCE_EXTI5_9,
    INTERRUPT_SOURCE_EXTI5_9,
    INTERRUPT_SOURCE_EXTI5_9,
    INTERRUPT_SOURCE_EXTI10_15,
    INTERRUPT_SOURCE_EXTI10_15,
    INTERRUPT_SOURCE_EXTI10_15,
    INTERRUPT_SOURCE_EXTI10_15,
    INTERRUPT_SOURCE_EXTI10_15,
    INTERRUPT_SOURCE_EXTI10_15,
};

static struct {
    callbackfn_t callback;
    void *aux_data;
} exti_handlers[NUM_EXTIINTERRUPTS];


void gpio_set_mode(const gpio_id_t portpin, moder_t mode)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);

    gpio->MODER &= ~(3<<(2*pin));
    gpio->MODER |= (mode<<(2*pin));
}

moder_t gpio_get_mode(const gpio_id_t portpin)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);

    return GET_BITS(gpio->MODER, 2*pin, 2);
}

void gpio_set_output_type(const gpio_id_t portpin, otype_t otype) {
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
    gpio->OTYPER &= ~(1 << pin); //reset to zero
    gpio->OTYPER |= (otype << pin);
}

void gpio_set_output_speed(const gpio_id_t portpin, ospeed_t ospeed) {
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
    gpio->OSPEEDR &= ~(3 << (2*pin)); //reset to zero
    gpio->OSPEEDR |= (ospeed << (2*pin));
}

void gpio_set_analog_switch(const gpio_id_t portpin) {
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
#if defined(STEFI_MCU_STM32F7)
    (void) gpio; (void) pin;   // F7: no analog switch register (ASCR is L4-only)
#else
    gpio->ASCR |= BIT(pin);
#endif
}

uint16_t gpio_get_port_from_portpin(const gpio_id_t portpin)
{
    return (portpin >> 8);
}

void gpio_set_pupd(const gpio_id_t portpin, pupdr_t pupd)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);

    gpio->PUPDR &= ~(3<<(2*pin));
    gpio->PUPDR |= (pupd<<(2*pin));
}

void gpio_set_alternate_function(const gpio_id_t portpin, afr_t af)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);

    // clear the 4-bit field first, so re-assigning a pin's AF works
    if (pin < 8)
    {
        gpio->AFR[0] &= ~(0xFu << (4*pin));
        gpio->AFR[0] |= (af << (4*pin));
    }
    else
    {
        gpio->AFR[1] &= ~(0xFu << (4*(pin-8)));
        gpio->AFR[1] |= (af << (4*(pin-8)));
    }
}


void gpio_enable_interrupt(const gpio_id_t portpin, const edge_t evt) {
    uint16_t pin = portpin & 0xFF;
    uint16_t port = (portpin >> 8);
    uint16_t shift = (pin%4)*4;
    peripheral_exti_enable();

    // Initialize EXTI0 for interrupt on PB0(EXTIx = PINx)
    SYSCFG->EXTICR[pin/4] &= ~(0xF << shift); // Clear settings
    SYSCFG->EXTICR[pin/4] |= (port << shift);

    EXTI->IMR1 |= BIT(pin); 		// Enable interrupt 0 (IM).
    switch (evt) {
        case FALLING_EDGE:
            EXTI->FTSR1 |= BIT(pin);
            break;
        case RISING_EDGE:
            EXTI->RTSR1 |= BIT(pin);
            break;
        case BOTH_EDGES:
            EXTI->RTSR1 |= BIT(pin);
            EXTI->FTSR1 |= BIT(pin);
            break;
    }
    interrupts_enable_source(extipin_to_irq[pin]);
}

void gpio_clear_interruptflag(uint8_t pin) {
    // PR is write-1-to-clear: a read-modify-write (|=) would also clear every
    // other pending line, so write the single bit.
    EXTI->PR1 = BIT(pin);
}

void gpio_set_interrupt_edge(const gpio_id_t portpin, const edge_t edge) {
    uint16_t pin = portpin & 0xFF;
    switch (edge) {
        case FALLING_EDGE:
            EXTI->RTSR1 &= ~BIT(pin);
            EXTI->FTSR1 |= BIT(pin);
            break;
        case RISING_EDGE:
            EXTI->FTSR1 &= ~BIT(pin);
            EXTI->RTSR1 |= BIT(pin);
            break;
        case BOTH_EDGES:
            EXTI->RTSR1 |= BIT(pin);
            EXTI->FTSR1 |= BIT(pin);
            break;
    }
}

nvic_source_t gpio_get_interrupt_source(const gpio_id_t portpin) {
    return extipin_to_irq[portpin & 0xF];
}

void gpio_interrupt_mask(const gpio_id_t portpin) {
    uint16_t pin = portpin & 0xFF;
    EXTI->IMR1 &= ~BIT(pin);
}

void gpio_interrupt_unmask(const gpio_id_t portpin) {
    uint16_t pin = portpin & 0xFF;
    // An edge seen while masked still latches PR: drop it, so unmasking
    // means "react to the next edge", not "to one that already happened".
    gpio_clear_interruptflag(pin);
    EXTI->IMR1 |= BIT(pin);
}

void gpio_interrupt_register_handler(const gpio_id_t portpin, callbackfn_t fn) {
    uint16_t pin = portpin & 0xFF;
    exti_handlers[pin].callback =fn;
    exti_handlers[pin].aux_data = NULL;
}

void gpio_interrupt_register_handler_context(const gpio_id_t portpin, callbackfn_t fn, void *aux) {
    uint16_t pin = portpin & 0xFF;
    exti_handlers[pin].callback =fn;
    exti_handlers[pin].aux_data = aux;
}

static void exti_dispatch(uint8_t exti_num)
{
    if (exti_handlers[exti_num].callback) {
        exti_handlers[exti_num].callback(exti_handlers[exti_num].aux_data);
    }
}

void EXTI0_IRQHandler(void)
{
    gpio_clear_interruptflag(0);
    exti_dispatch(0);
}

void EXTI1_IRQHandler(void)
{
    gpio_clear_interruptflag(1);
    exti_dispatch(1);
}

void EXTI2_IRQHandler(void)
{
    gpio_clear_interruptflag(2);
    exti_dispatch(2);
}

void EXTI3_IRQHandler(void)
{
    gpio_clear_interruptflag(3);
    exti_dispatch(3);
}

void EXTI4_IRQHandler(void)
{
    gpio_clear_interruptflag(4);
    exti_dispatch(4);
}

void EXTI5_9_IRQHandler(void) {
    for(uint8_t i = 5; i <= 9; i++) {
        if(EXTI->PR1 & BIT(i)) {
            gpio_clear_interruptflag(i);
            exti_dispatch(i);
        }
    }
}

void EXTI10_15_IRQHandler(void) {
    for(uint8_t i = 10; i <= 15; i++) {
        if(EXTI->PR1 & BIT(i)) {
            gpio_clear_interruptflag(i);
            exti_dispatch(i);
        }
    }
}