#include "button.h"

#include <stddef.h>

#include "libstefi/timer.h"

#define DEBOUNCE 1

//Not used, assume multiple press not possible in debounce time
typedef enum {
    BUTTON_STABLE,
    BUTTON_IN_DEBOUNCE,
} button_state_t;

typedef struct {
    stefi_button_t name;
    callbackfn_t cb;
} button_context_t;

static button_context_t button_ctx[STEFI_MAX_BUTTONS];
static stefi_button_t button_to_debounce;
static callbackfn_t debounce_callback;

void button_init(stefi_button_t id) {
    peripheral_gpio_enable(gpio_get_port_from_portpin(buttons[id].portpin));
    gpio_set_mode(buttons[id].portpin, MODER_INPUT);
    gpio_set_pupd(buttons[id].portpin, buttons[id].pull);
}

bool button_is_pressed(stefi_button_t id) {
    sig_t button_state = gpio_read(buttons[id].portpin);
    if(buttons[id].pull != PULL_DOWN) {
        return button_state ? false : true;
    } else {
        return button_state ? true : false;
    }
}

static void debounced_reaction() {
    timer_stop(hardware_timers[DEBOUNCE_TIMER].timer);
    if(button_is_pressed(button_to_debounce)) {
        debounce_callback((void *) button_to_debounce);
    }
}

static void debounce_start(void *aux) {
    button_context_t *ctx = (button_context_t *) aux;
    button_to_debounce = ctx->name;
    debounce_callback = ctx->cb;
    timer_start(hardware_timers[DEBOUNCE_TIMER].timer);
}

void button_interrupt_init(stefi_button_t id, callbackfn_t fn) {
    //Falling Edge Hard-Coded
    if(!DEBOUNCE) {
        gpio_enable_interrupt(buttons[id].portpin, FALLING_EDGE);
        gpio_interrupt_register_handler(buttons[id].portpin, fn);
    } else {
        //register callback fn
        button_ctx[id].name = id;
        button_ctx[id].cb = fn;
        gpio_enable_interrupt(buttons[id].portpin, (buttons[id].pull == PULL_DOWN) ? RISING_EDGE : FALLING_EDGE);
        gpio_interrupt_register_handler_context(buttons[id].portpin, debounce_start, &button_ctx[id]);
        //debounce_init to 40ms, done in board init
        static bool initialized = false;
        if (initialized) return;
        initialized = true;
        //timer_update_interrupt_register_handler(hardware_timers[DEBOUNCE_TIMER].timer, debounced_reaction);
        timer_update_interrupt_register_handler(hardware_timers[DEBOUNCE_TIMER].timer, debounced_reaction);
        timer_enable_interrupt(hardware_timers[DEBOUNCE_TIMER].timer);
    }
}