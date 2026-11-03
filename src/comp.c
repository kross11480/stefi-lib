/**********************************************************************/
/* COMP (STM32L4, RM0351 §22)                                         */
/**********************************************************************/
#include "libstefi/comp.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/gpio_internal.h"
#include "internal/rcc_internal.h"

#include <stddef.h>

#define COMP_CSR(n) (*(volatile uint32_t *) (0x40010200u + 4u * ((n) - 1u)))
#define CSR_EN      BIT(0)
#define CSR_BRGEN   BIT(22)   /* resistor bridge: 1/4, 1/2, 3/4 VREFINT */
#define CSR_SCALEN  BIT(23)   /* VREFINT scaler: any VREFINT input */
#define CSR_VALUE   (1u << 30)

static struct { callbackfn_t fn; void *aux; } handlers[2];

static const gpio_id_t inp_pins[2][2] = {{C5, B2}, {B4, B6}};

void comp_init(const comp_config_t *cfg)
{
    uint8_t n = cfg->instance;
    RCC->APB2ENR |= BIT(0);                         // SYSCFGEN (COMP + VREFBUF clock)
    gpio_id_t pin = inp_pins[n - 1u][cfg->inp & 1u];
    peripheral_gpio_enable(gpio_get_port_from_portpin(pin));
    gpio_set_mode(pin, MODER_ANALOG);
    gpio_set_analog_switch(pin);

    uint32_t csr = ((uint32_t) cfg->power << 2) | ((uint32_t) (cfg->inm & 7u) << 4)
                 | ((uint32_t) (cfg->inp & 1u) << 7) | ((uint32_t) (cfg->hysteresis & 3u) << 16)
                 | (cfg->invert ? BIT(15) : 0u);
    if (cfg->inm <= COMP_INM_VREFINT) csr |= CSR_SCALEN;          // as ST's HAL
    if (cfg->inm < COMP_INM_VREFINT) csr |= CSR_BRGEN;
    COMP_CSR(n) = csr;
    COMP_CSR(n) = csr | CSR_EN;
    for (volatile uint32_t i = 0; i < 2000u; i++);  // tSTART (ultra-low power: ~80 us)
}

bool comp_output(uint8_t instance)
{
    return (COMP_CSR(instance) & CSR_VALUE) != 0;
}

void comp_enable_interrupt(uint8_t instance, bool rising, bool falling, callbackfn_t fn, void *aux)
{
    handlers[instance - 1u].fn = fn;
    handlers[instance - 1u].aux = aux;
    uint32_t line = BIT(20 + instance);              // EXTI 21 / 22
    if (rising) EXTI->RTSR1 |= line; else EXTI->RTSR1 &= ~line;
    if (falling) EXTI->FTSR1 |= line; else EXTI->FTSR1 &= ~line;
    EXTI->PR1 = line;
    EXTI->IMR1 |= line;
    interrupts_enable_source((nvic_source_t) 64);    // COMP
}

void COMP_IRQHandler(void)
{
    for (uint8_t n = 1; n <= 2; n++) {
        uint32_t line = BIT(20 + n);
        if (EXTI->PR1 & line) {
            EXTI->PR1 = line;
            if (handlers[n - 1u].fn) handlers[n - 1u].fn(handlers[n - 1u].aux);
        }
    }
}
