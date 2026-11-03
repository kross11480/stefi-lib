#pragma once

#include "../src/internal/gpio_internal.h"

#include "interrupts.h"
#include "stdint.h"

/**********************************************************************/
/* GPIO (General Purpose Input and Output) Hardware Abstraction layer */
/**********************************************************************/
typedef enum
{
    A0 = 0x000, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15,
    B0 = 0x100, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14, B15,
    C0 = 0x200, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15,
    D0 = 0x300, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15,
    E0 = 0x400, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15,
    //port F (e.g. PF0/PF1 on the G431KB: Rohmi motor DIR, AN; PF2..PF15 on the F767 scope)
    F0 = 0x500, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
    G0 = 0x600, G1, G2, G3, G4, G5, G6, G7, G8, G9, G10, G11, G12, G13, G14, G15,
} gpio_id_t;

typedef enum {LOW = 0, HIGH = 1} sig_t;
typedef enum {MODER_INPUT = 0, MODER_OUTPUT = 1, MODER_AF = 2, MODER_ANALOG = 3} moder_t;
typedef enum {PUSH_PULL = 0, OPEN_DRAIN = 1} otype_t;
typedef enum {LOW_S = 0, MEDIUM_S = 1, HIGH_S = 2, VERY_HIGH_S = 3} ospeed_t;
typedef enum {NONE = 0, PULL_UP = 1, PULL_DOWN = 2} pupdr_t;
typedef enum {AF0 = 0, AF1, AF2, AF3, AF4, AF5, AF6, AF7, AF8, AF9, AF10, AF11, AF12, AF13, AF14, AF15,} afr_t;
typedef enum {RISING_EDGE = 0, FALLING_EDGE = 1, BOTH_EDGES = 2} edge_t;

/* GPIO initialization functions*/
void gpio_set_mode(gpio_id_t portpin, moder_t mode);
void gpio_set_output_type(gpio_id_t portpin, otype_t otype);
void gpio_set_output_speed(gpio_id_t portpin, ospeed_t ospeed);
void gpio_set_pupd(gpio_id_t portpin, pupdr_t pupd);
void gpio_set_alternate_function(gpio_id_t portpin, afr_t af);
void gpio_set_analog_switch(gpio_id_t portpin);

/* GPIO processing functions: read, write, toggle  */
static inline GPIO_typeDef * gpio_get_base_address(gpio_id_t portpin)
{
    uint16_t port = (portpin >> 8);
    uint32_t baseoffset = port * GPIO_PORTOFFSET;
    return (GPIO_typeDef *) (GPIO_BASE +  baseoffset);
}

static inline sig_t gpio_read(gpio_id_t portpin)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
    return (gpio->IDR & (1<<pin) ? HIGH : LOW);
}

static inline void gpio_write(gpio_id_t portpin, sig_t val)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
    //low 16 bit odr high, upper 16 bit odr low
    gpio->BSRR = (1 << pin)<<( val ? 0: 16);
}

static inline void gpio_toggle(gpio_id_t portpin)
{
    uint16_t pin = portpin & 0xFF;
    GPIO_typeDef *gpio = gpio_get_base_address(portpin);
    gpio->ODR ^= (1 << pin);
}

/* GPIO Interrupt functions (extended interrupts mapped to GPIO Pins)*/
void gpio_enable_interrupt(gpio_id_t portpin,  edge_t edge);
void gpio_interrupt_register_handler(gpio_id_t portpin, callbackfn_t);
void gpio_interrupt_register_handler_context(const gpio_id_t portpin, callbackfn_t fn, void *aux);
/* Select exactly one edge (or both) for an EXTI line already set up with
 * gpio_enable_interrupt(); the other edge is cleared. */
void gpio_set_interrupt_edge(gpio_id_t portpin, edge_t edge);
/* Gate an EXTI line without touching its routing/edge. Unmask drops any edge
 * latched while masked, so only the next edge fires. */
void gpio_interrupt_mask(gpio_id_t portpin);
void gpio_interrupt_unmask(gpio_id_t portpin);
/* NVIC line serving a pin's EXTI interrupt (e.g. for interrupts_set_priority). */
nvic_source_t gpio_get_interrupt_source(gpio_id_t portpin);

/* For testing*/
moder_t gpio_get_mode(gpio_id_t portpin);
uint16_t gpio_get_port_from_portpin(gpio_id_t portpin);


