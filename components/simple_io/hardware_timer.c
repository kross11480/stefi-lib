#include "hardware_timer.h"

#include <stddef.h>

#include "libstefi/system.h"

/* Prescalers from the current core clock (4 MHz at reset on the L476, 16
 * MHz on the G431, 80/170 MHz after system_init*): a 10 kHz tick for
 * periodic timers, ~1 kHz PWM with 256 steps. At 4 MHz this gives the
 * same 1 ms / 1 kHz as the original fixed values. */
static uint16_t tick_10khz_prescaler(void) { return (uint16_t) (system_get_core_clock() / 10000u); }
static uint16_t pwm_1khz_prescaler(void)   { return (uint16_t) ((system_get_core_clock() + 128000u) / 256000u); }

void htimer_init(htimer_id_t id, htimer_channel_t channel, htimer_mode_t mode, uint32_t period_us, htimer_callbackfn_t cb) {
    timer_init(id);
    switch(mode) {
        case HTIMER_MODE_PERIODIC:
            timer_set_period(id, tick_10khz_prescaler(), period_us * 10u); //period in ms
            break;
        case HTIMER_MODE_PWM:
            timer_set_period(id, pwm_1khz_prescaler(), 255); //PWM Freq ~ 1 KHZ
            timer_set_mode_pwm(id, channel);
            timer_cc_enable(id, channel, false);
            timer_set_compare(id, channel, 0);
            break;
        case HTIMER_MODE_INPUTCAPTURE:
            timer_set_mode_ic(id, channel);
            timer_cc_enable(id, channel, true);
            break;
        case HTIMER_MODE_ONE_SHOT:
            break;
    }

    if (channel == HTIMER_NOCHANNEL) {
        timer_update_interrupt_register_handler(id, cb);
        timer_enable_interrupt(id);
    } else {
        timer_cc_interrupt_register_handler(id, channel, cb);
    }
}

void htimer_init_periodic_ms(htimer_id_t id, uint32_t period_ms, htimer_callbackfn_t cb) {
    htimer_init(id, HTIMER_NOCHANNEL, HTIMER_MODE_PERIODIC, period_ms, cb);
}

void htimer_init_pwm(htimer_id_t id, htimer_channel_t channel) {
    htimer_init(id, channel, HTIMER_MODE_PWM, 0, NULL);
    timer_set_compare(id, channel, 0);
}

void htimer_init_input_capture(htimer_id_t id, htimer_channel_t channel, htimer_callbackfn_t cb) {
    htimer_init(id, channel, HTIMER_MODE_INPUTCAPTURE, 0, cb);
}

void htimer_start(htimer_id_t id) {
    timer_start(id);
}

void htimer_stop(htimer_id_t id) {
    timer_stop(id);
}

void htimer_set_pwm(htimer_id_t id, htimer_channel_t channel, uint32_t duty_0_255) {
    timer_set_compare(id, channel, duty_0_255);
}

uint32_t htimer_get_overflow(htimer_id_t id) {
    return timer_get_arr(id);
}

uint32_t htimer_get_count(htimer_id_t id) {
    return timer_get_count(id);
}

uint32_t htimer_get_capture(htimer_id_t id, htimer_channel_t channel) {
    return timer_get_compare(id, channel);
}