#include "edge_trigger.h"

#include "stefi_board.h"

#include <stddef.h>

static struct {
    edge_t edge;
    volatile bool armed;
    callbackfn_t fn;
    void *aux;
} trig = {.edge = RISING_EDGE};

static void on_exti(void *unused)
{
    (void) unused;
    if (!trig.armed) return;

    sig_t level = gpio_read(edge_trigger.portpin);
    bool valid = (trig.edge == RISING_EDGE) ? (level == HIGH) : (level == LOW);
    if (!valid) return;   // glitch: the level is already back

    edge_trigger_disarm();
    if (trig.fn) trig.fn(trig.aux);
}

void edge_trigger_init(void)
{
    gpio_id_t pin = edge_trigger.portpin;
    peripheral_gpio_enable(gpio_get_port_from_portpin(pin));
    gpio_set_mode(pin, MODER_INPUT);
    gpio_set_pupd(pin, edge_trigger.pull);

    gpio_interrupt_register_handler_context(pin, on_exti, NULL);
    gpio_enable_interrupt(pin, trig.edge);   // routes EXTI + NVIC ...
    gpio_interrupt_mask(pin);                // ... but stays disarmed
    gpio_set_interrupt_edge(pin, trig.edge);
    edge_trigger_set_priority(1);
    trig.armed = false;
}

void edge_trigger_set_edge(edge_t edge)
{
    trig.edge = (edge == FALLING_EDGE) ? FALLING_EDGE : RISING_EDGE;
    gpio_set_interrupt_edge(edge_trigger.portpin, trig.edge);
}

edge_t edge_trigger_get_edge(void)
{
    return trig.edge;
}

void edge_trigger_arm(callbackfn_t fn, void *aux)
{
    trig.fn = fn;
    trig.aux = aux;
    trig.armed = true;
    gpio_interrupt_unmask(edge_trigger.portpin);   // drops stale edges
}

void edge_trigger_disarm(void)
{
    gpio_interrupt_mask(edge_trigger.portpin);
    trig.armed = false;
}

bool edge_trigger_is_armed(void)
{
    return trig.armed;
}

void edge_trigger_set_priority(uint8_t priority)
{
    interrupts_set_priority(gpio_get_interrupt_source(edge_trigger.portpin), priority);
}
