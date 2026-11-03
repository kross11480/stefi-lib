#include <stdint.h>
#include <stdbool.h>
#include "libstefi/interrupts.h"
#include "libstefi/util.h"
#include "internal/nvic_internal.h"

#define NUM_INTERRUPTS 240

static struct {
    struct {
        callbackfn_t callback;
        void *aux_data; //use in case of gpio, timer modes like interrupt in advanced modes later
    } handlers[NUM_INTERRUPTS];
    bool initialized;
} module;

static inline uint8_t get_irq_num(void)
{
    uint8_t irq;
    __asm volatile ("mrs %0, ipsr" : "=r" (irq));
    return irq - 16;
}

void generic_dispatch()
{
    uint8_t irq_num = get_irq_num();
    module.handlers[irq_num].callback(module.handlers[irq_num].aux_data);
}

//Usage: interrupts_register_handler(TIMx_IRQn, tim_irq_handler);
void interrupts_register_handler(nvic_source_t source, callbackfn_t fn)
{
    module.handlers[source].callback = fn;
}

void interrupts_init(void)
{
    for(uint8_t i = 0; i < NUM_INTERRUPTS; i++)
    {
        module.handlers[i].callback = NULL;
    }
    module.initialized = true;
}

void interrupts_global_enable(void)
{
    NVIC_GlobalEnable();
}

void interrupts_global_disable(void)
{
    NVIC_GlobalDisable();
}

void interrupts_enable_source(nvic_source_t irq_num)
{
    assert(irq_num >= 0);
    NVIC_BASE->ISER[irq_num/32] |= (1 << irq_num%32);
}

void interrupts_set_priority(nvic_source_t irq_num, uint8_t priority)
{
    assert(irq_num >= 0);
    // NVIC_IPR: one byte per IRQ, only the top 4 bits are implemented.
    volatile uint8_t *ipr = (volatile uint8_t *) 0xE000E400u;
    ipr[irq_num] = (uint8_t) ((priority & 0xFu) << 4);
}

void interrupts_disable_source(nvic_source_t irq_num)
{
    NVIC_BASE->ICER[irq_num / 32] |= (1U << (irq_num % 32));
}

