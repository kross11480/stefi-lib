#pragma once

#include "libstefi/timer.h"

typedef tim_id_t htimer_id_t;
typedef void (*htimer_callbackfn_t)();

typedef enum {
    HTIMER_MODE_PERIODIC,
    HTIMER_MODE_PWM,
    HTIMER_MODE_ONE_SHOT,
    HTIMER_MODE_INPUTCAPTURE,
} htimer_mode_t;

typedef enum {
    HTIMER_NOCHANNEL = 0,
    HTIMER_CHANNEL_1 = TIMER_CHANNEL1,
    HTIMER_CHANNEL_2 = TIMER_CHANNEL2,
    HTIMER_CHANNEL_3 = TIMER_CHANNEL3,
    HTIMER_CHANNEL_4 = TIMER_CHANNEL4,
} htimer_channel_t;

/* period_ms up to 6553 on 16-bit timers (10 kHz tick at any core clock) */
void htimer_init_periodic_ms(htimer_id_t id, uint32_t period_ms, htimer_callbackfn_t cb);
void htimer_init_pwm(htimer_id_t id, htimer_channel_t channel);
void htimer_init_input_capture(htimer_id_t id, htimer_channel_t channel, htimer_callbackfn_t cb);

// Control methods
void htimer_start(htimer_id_t id);
void htimer_stop(htimer_id_t id);

//set pwm duty cycle
void htimer_set_pwm(htimer_id_t id, htimer_channel_t channel, uint32_t duty_0_255);

uint32_t htimer_get_overflow(htimer_id_t id);
uint32_t htimer_get_count(htimer_id_t id);
uint32_t htimer_get_capture(htimer_id_t id, htimer_channel_t channel);