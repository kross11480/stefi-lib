/*********************************************************************/
/* Timer Hardware Abstraction layer*/
/*********************************************************************/
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "interrupts.h"

typedef enum {
    TIMER_INVALID = 0,
    TIMER1 = 1,
    TIMER2 = 2,
    TIMER3 = 3,
    TIMER4 = 4,
    //TIMER5 Not available in tested boards
    TIMER6 = 6,
    TIMER7 = 7,
    TIMER8 = 8,
    //TIMER9 to 14 Not available in tested boards
    TIMER15 = 15,
    TIMER16 = 16,
    TIMER17 = 17,
} tim_id_t;

typedef enum {
    TIMER_UPDATE_IRQ,
    TIMER_CC1_IRQ,
    TIMER_CC2_IRQ,
    TIMER_CC3_IRQ,
    TIMER_CC4_IRQ,
    TIMER_IRQ_MAX,
} timer_interrupt_t;

typedef enum {
    TIMER_CHANNEL1 = 1,
    TIMER_CHANNEL2 = 2,
    TIMER_CHANNEL3 = 3,
    TIMER_CHANNEL4 = 4,
} timer_channel_t;

// Edge polarity for input capture
typedef enum {
    TIMER_EDGE_RISING,
    TIMER_EDGE_FALLING,
} timer_edge_t;

/*--------------------------*/
/* Initialization & Control */
/*--------------------------*/
void timer_init(const tim_id_t);
void timer_start(const tim_id_t);
void timer_stop(const tim_id_t);
void timer_reset(const tim_id_t);

/*-----------------------*/
/* Counter & Auto-Reload */
/*-----------------------*/
uint32_t timer_get_count(const tim_id_t);
void timer_set_count(const tim_id_t, uint32_t cnt);
uint32_t timer_get_arr(const tim_id_t);
//set prescaler and ARR according to period or delay
void timer_set_period(const tim_id_t timer, uint16_t prescaler, uint32_t period);

/*--------------------*/
/* Channel Control    */
/*--------------------*/
void timer_cc_enable(const tim_id_t timer_id, timer_channel_t channel, bool is_input);

/*-----------------------------------*/
/* PWM Configuration         */
/*-----------------------------------*/
uint32_t timer_get_compare(const tim_id_t timer_id, timer_channel_t channel);
void timer_set_compare(const tim_id_t timer_id, timer_channel_t channel, uint32_t duty);
void timer_set_mode_pwm(const tim_id_t timer_id, timer_channel_t channel);

/*-----------------------------------*/
/* Input Capture Configuration       */
/*-----------------------------------*/
void timer_set_mode_ic(const tim_id_t timer_id, timer_channel_t channel);
void timer_set_ic_edge(const tim_id_t timer_id, timer_channel_t channel, timer_edge_t edge);

/*-----------------------------------*/
/* One pulse mode Configuration      */
/*-----------------------------------*/
void timer_set_mode_opm(const tim_id_t timer_id);
/**
 * @brief Block until update flag (UIF) is set
 */
void timer_wait_for_update_flag(const tim_id_t timer);

/*-----------------------------------*/
/* Output polarity, synchronisation  */
/*-----------------------------------*/
/* timer_cc_enable() makes outputs active-low (the STefi LEDs sink); ST's
 * examples and most loads want active-high: call this after it. */
void timer_set_oc_polarity(const tim_id_t timer_id, timer_channel_t channel, bool active_high);

typedef enum {
    TIMER_TRGO_RESET = 0, TIMER_TRGO_ENABLE = 1, TIMER_TRGO_UPDATE = 2, TIMER_TRGO_OC1 = 3,
    TIMER_TRGO_OC1REF = 4, TIMER_TRGO_OC2REF = 5, TIMER_TRGO_OC3REF = 6, TIMER_TRGO_OC4REF = 7,
} timer_trgo_t;

typedef enum {
    TIMER_SLAVE_DISABLED = 0, TIMER_SLAVE_RESET = 4, TIMER_SLAVE_GATED = 5,
    TIMER_SLAVE_TRIGGER = 6, TIMER_SLAVE_EXTERNAL_CLOCK = 7,
} timer_slave_mode_t;

/* Trigger inputs (SMCR.TS). Which timer an ITRx is, is fixed per slave by
 * the MCU (RM0440 table 'TIMx internal trigger connection'; e.g. on the G4
 * TIM3: ITR1 = TIM2, TIM4: ITR2 = TIM3). */
typedef enum {
    TIMER_TS_ITR0 = 0, TIMER_TS_ITR1 = 1, TIMER_TS_ITR2 = 2, TIMER_TS_ITR3 = 3,
    TIMER_TS_TI1F_ED = 4, TIMER_TS_TI1FP1 = 5, TIMER_TS_TI2FP2 = 6, TIMER_TS_ETRF = 7,
} timer_trigger_t;

/** Master: what drives TRGO; with sync, MSM delays this timer's own
 *  trigger so it starts in step with its slaves. */
void timer_set_master_mode(const tim_id_t timer_id, timer_trgo_t trgo, bool sync_slaves);
void timer_set_slave_mode(const tim_id_t timer_id, timer_slave_mode_t mode, timer_trigger_t trigger);

/** Input capture source: direct = TIx -> ICx, indirect = the neighbour
 *  (TI2 -> IC1, TI1 -> IC2...) — two channels on one pin = PWM input. */
void timer_set_ic_source(const tim_id_t timer_id, timer_channel_t channel, bool indirect);

/** Advanced timers (TIM1/8/15-17): update event every n+1 periods. */
void timer_set_repetition(const tim_id_t timer_id, uint8_t n);

/** Address of CCRx — target for DMA (e.g. waveform from memory). */
volatile uint32_t *timer_ccr_address(const tim_id_t timer_id, timer_channel_t channel);

/** UG: reload prescaler/ARR now, reset the counter. */
void timer_generate_update(const tim_id_t timer_id);

/*-----------------------------------*/
/* DMA requests                      */
/*-----------------------------------*/
typedef enum {
    TIMER_DMA_UPDATE = 8,   /* UDE: one DMA request per update event */
    TIMER_DMA_CC1 = 9,
    TIMER_DMA_CC2 = 10,
    TIMER_DMA_CC3 = 11,
    TIMER_DMA_CC4 = 12,
} timer_dma_request_t;

/**
 * @brief Let the timer raise a DMA request on the given event (TIMx_DIER).
 * Which DMA channel/stream serves it is fixed by the MCU's request table.
 */
void timer_enable_dma_request(const tim_id_t timer_id, timer_dma_request_t request);
void timer_disable_dma_request(const tim_id_t timer_id, timer_dma_request_t request);

/*--------------------*/
/* Interrupts */
/*--------------------*/
void timer_enable_interrupt(const tim_id_t);
/** NVIC line of a timer's update interrupt (e.g. for interrupts_set_priority). */
nvic_source_t timer_get_interrupt_source(const tim_id_t);
void timer_disable_interrupt(const tim_id_t);
void timer_update_interrupt_register_handler(const tim_id_t timer_id, callbackfn_t fn);
void timer_cc_interrupt_register_handler(const tim_id_t timer_id, timer_interrupt_t channel, callbackfn_t fn);