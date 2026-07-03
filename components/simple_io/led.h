#pragma once

#include "stefi_board.h"
#include  "stdint.h"

typedef enum {
    OUTPUT,
    PWM
} pin_mode;

//Functions
void led_init(stefi_led_t id, pin_mode mode);
void led_on(stefi_led_t id);
void led_off(stefi_led_t id);
void led_toggle(stefi_led_t);
void led_set_brightness(stefi_led_t id, uint32_t duty_0_255);