#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "libstefi/dma.h"

/**********************************************************************/
/* Channel DMA (STM32L4 / STM32G4) Hardware Abstraction layer         */
/*                                                                    */
/* Configurable peripheral/memory transfers on the channel-based DMA  */
/* controllers, with request routing for both families:               */
/*   L4: DMA_CSELR selector per channel (RM0351 table 41/42),         */
/*       e.g. TIM1_UP = DMA1 channel 6, request 7                     */
/*   G4: DMAMUX request ID, any channel (RM0440 table 91),            */
/*       e.g. TIM1_UP = 46                                            */
/* The family is detected at run time, so `request` is simply the     */
/* value of the MCU the board is built for.                           */
/*                                                                    */
/* Only available when building for STEFI_MCU_STM32L4G4 (the F7 uses  */
/* libstefi/dma_stream.h).                                            */
/**********************************************************************/
#if !defined(STEFI_MCU_STM32L4G4)
#error "libstefi/dma_channel.h is the L4/G4 channel DMA; use libstefi/dma_stream.h on the F7"
#endif

/* Per-channel event flags (normalised to channel-1 positions of DMA_ISR). */
#define DMA_CHANNEL_FLAG_GI (1u << 0)   /* any of the below */
#define DMA_CHANNEL_FLAG_TC (1u << 1)   /* transfer complete */
#define DMA_CHANNEL_FLAG_HT (1u << 2)   /* half transfer     */
#define DMA_CHANNEL_FLAG_TE (1u << 3)   /* transfer error    */

typedef struct {
    peripheral_dma_t dma;      /* PERIPHERAL_DMA1 / PERIPHERAL_DMA2 */
    uint8_t channel;           /* 1-based */
    uint8_t request;           /* L4: CSELR value, G4: DMAMUX request ID (ignored for M2M) */
    dma_dir_t dir;
    dma_width_t periph_width;
    dma_width_t mem_width;
    bool periph_inc;
    bool mem_inc;
    bool circular;
    dma_priority_t priority;
    bool irq_tc;
    bool irq_ht;
    bool irq_te;
} dma_channel_config_t;

/* Callback in interrupt context; flags = DMA_CHANNEL_FLAG_* (already cleared).
 * Interrupts are wired for DMA1 channels 1..7. */
typedef void (*dma_channel_callback_t)(void *aux, uint32_t flags);

/**
 * @brief Configure a channel (disabled first, flags cleared, request routed).
 * @param periph  peripheral register (CPAR); for M2M: the source
 * @param mem     memory buffer (CMAR); for M2M: the destination
 * @param items   number of items (CNDTR, 1..65535)
 * The channel is left disabled; start it with dma_channel_enable().
 */
void dma_channel_configure(const dma_channel_config_t *cfg, volatile const void *periph,
                           void *mem, uint16_t items);

void dma_channel_enable(peripheral_dma_t dma, uint8_t channel);
void dma_channel_disable(peripheral_dma_t dma, uint8_t channel);
bool dma_channel_is_enabled(peripheral_dma_t dma, uint8_t channel);
uint16_t dma_channel_get_items(peripheral_dma_t dma, uint8_t channel);

void dma_channel_register_handler(peripheral_dma_t dma, uint8_t channel,
                                  dma_channel_callback_t fn, void *aux);

/** NVIC source of a DMA1 channel (for interrupts_set_priority()). */
nvic_source_t dma_channel_irq(peripheral_dma_t dma, uint8_t channel);
