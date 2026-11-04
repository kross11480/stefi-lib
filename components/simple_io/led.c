#include "libstefi/peripheral.h"

#include "led.h"

#include "libstefi/system.h"

//Functions
void led_init(stefi_led_t id, pin_mode mode) {
    uint16_t port = gpio_get_port_from_portpin(leds[id].portpin);
    peripheral_gpio_enable(port);

    switch(mode) {
        case OUTPUT:
            gpio_write(leds[id].portpin, leds[id].off_state);
            gpio_set_mode(leds[id].portpin, MODER_OUTPUT);
        break;
        case PWM:
            gpio_set_mode(leds[id].portpin, MODER_AF);
            gpio_set_alternate_function(leds[id].portpin, leds[id].af_mode);
            timer_init(leds[id].timer_id);
            //PWM Freq ~ 1 KHZ at any core clock (16 at the 4 MHz reset clock)
            timer_set_period(leds[id].timer_id,
                             (uint16_t) ((system_get_core_clock() + 128000u) / 256000u), 255);
            timer_set_mode_pwm(leds[id].timer_id, leds[id].channel);
            timer_cc_enable(leds[id].timer_id, leds[id].channel, false);
            timer_set_compare(leds[id].timer_id, leds[id].channel, 0);
            timer_start(leds[id].timer_id);
            break;
        default:
            break;
    }
}

void led_on(stefi_led_t id) {
    gpio_write(leds[id].portpin, !leds[id].off_state);
}

void led_off(stefi_led_t id) {
    gpio_write(leds[id].portpin, leds[id].off_state);
}

void led_toggle(stefi_led_t id) {
    gpio_toggle(leds[id].portpin);
}

void led_set_brightness(stefi_led_t id, uint32_t duty_0_255) {
    timer_set_compare(leds[id].timer_id, leds[id].channel, duty_0_255);
}