/*
 * The hardware actions behind the states (V1: Core/Src/app.c), expressed
 * with stefi components instead of register code.
 */
#include "scope.h"

#include "board.h"
#include "button.h"
#include "dsp.h"
#include "edge_trigger.h"
#include "led.h"
#include "parallel_adc.h"
#include "ref_dac.h"

#include "libstefi/interrupts.h"
#include "libstefi/usb_cdc.h"

#include <stddef.h>

scope_t scope;

/* Raw GPIO snapshots from the DMA, converted in place to int16 samples. */
static uint16_t sample_buffer[SCOPE_NUM_SAMPLES];
#if !SCOPE_FILTER_INPLACE
static int16_t filtered_buffer[SCOPE_NUM_SAMPLES];
#endif

static void on_capture_done(void *aux)
{
    (void) aux;
    fsm_post(&scope_fsm, EV_SAMPL_DONE);
}

static void on_trigger(void *aux)
{
    (void) aux;
    parallel_adc_start();   // in the EXTI ISR: capture starts with the next clock
}

static void on_button(void *aux)
{
    (void) aux;
    fsm_post(&scope_fsm, EV_B1_PRESSED);
}

void scope_hw_init(void)
{
    parallel_adc_init();
    ref_dac_init();
    edge_trigger_init();
    button_interrupt_init(BUTTON_USER, on_button);
    // NVIC: capture DMA 0 and trigger 1 (set by the components), USB 6,
    // the user button (EXTI + debounce timer) last
    interrupts_set_priority(gpio_get_interrupt_source(buttons[BUTTON_USER].portpin), 8);
    interrupts_set_priority(timer_get_interrupt_source(hardware_timers[DEBOUNCE_TIMER].timer), 8);
    scope_reset_config();
}

void scope_reset_config(void)
{
    scope.req_rate_hz = SCOPE_DEFAULT_RATE_HZ;
    scope.req_vref_mv = SCOPE_DEFAULT_VREF_MV;
    scope.req_movavg_points = SCOPE_DEFAULT_MOVAVG;
    scope_apply_sample_rate();
    scope_apply_reference();
    scope_apply_trigger_edge(RISING_EDGE);
    scope_apply_movavg();
    scope.overflow = false;
}

/* Bit i -> LED i, for as many LEDs as the board has (F767: LD1..LD3 show
 * the whole state number; L476: only bit 0 on LD2). */
void scope_status_leds(uint8_t pattern)
{
    for (uint32_t i = 0; i < 3 && i < NUM_LEDS; i++) {
        if (pattern & (1u << i)) led_on(i); else led_off(i);
    }
}

void scope_apply_sample_rate(void)
{
    scope.rate_hz = parallel_adc_set_sample_rate(scope.req_rate_hz);
}

void scope_apply_reference(void)
{
    uint32_t mv = scope.req_vref_mv > SCOPE_VREF_FULL_MV ? SCOPE_VREF_FULL_MV : scope.req_vref_mv;
    // LTC1450 output is subtracted from 4.096 V: Vref = 4095 mV - code
    ref_dac_write((uint16_t) (SCOPE_VREF_FULL_MV - mv));
    scope.vref_mv = mv;
}

void scope_apply_trigger_edge(edge_t edge)
{
    edge_trigger_set_edge(edge);
    scope.trigger_edge = edge;
}

void scope_apply_movavg(void)
{
    uint32_t p = scope.req_movavg_points;
    // off: 0, even, or wider than the record (as in V1)
    scope.movavg_points = (p == 0 || p > SCOPE_NUM_SAMPLES || (p % 2u) == 0) ? 0 : p;
#if SCOPE_FILTER_INPLACE
    if (p > DSP_MOVAVG_INPLACE_MAX_WINDOW) scope.movavg_points = 0;
#endif
}

void scope_acquisition_arm(void)
{
    parallel_adc_prepare(sample_buffer, SCOPE_NUM_SAMPLES, on_capture_done, NULL);
    edge_trigger_arm(on_trigger, NULL);
}

void scope_manual_trigger(void)
{
    // atomic against the trigger ISR doing the same
    interrupts_global_disable();
    edge_trigger_disarm();
    parallel_adc_start();   // no-op if the trigger already started it
    interrupts_global_enable();
}

void scope_acquisition_stop(void)
{
    edge_trigger_disarm();
    if (parallel_adc_busy()) parallel_adc_abort();
}

void scope_preprocess(void)
{
    int16_t *samples = (int16_t *) sample_buffer;
    scope.overflow = parallel_adc_convert(sample_buffer, samples, SCOPE_NUM_SAMPLES);
#if SCOPE_FILTER_INPLACE
    scope.filtered = scope.movavg_points > 1u
            && dsp_moving_average_i16_inplace(samples, SCOPE_NUM_SAMPLES, scope.movavg_points);
#else
    scope.filtered = scope.movavg_points > 1u
            && dsp_moving_average_i16(samples, filtered_buffer, SCOPE_NUM_SAMPLES,
                                      scope.movavg_points);
#endif
}

void scope_send_data(void)
{
#if SCOPE_FILTER_INPLACE
    const void *data = sample_buffer;   // filtered in place, if at all
#else
    const void *data = scope.filtered ? (const void *) filtered_buffer
                                      : (const void *) sample_buffer;
#endif
    usb_cdc_write(data, SCOPE_NUM_SAMPLES * sizeof(int16_t));
    const uint8_t ovf = scope.overflow ? 0xFF : 0x00;
    usb_cdc_write(&ovf, 1);
}
