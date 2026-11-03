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