#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "libstefi/dma.h"

/**********************************************************************/
/* Stream DMA (STM32F7) Hardware Abstraction layer                    */
/*                                                                    */
/* The F7 DMA controllers have 8 *streams* each; a stream is routed   */
/* to one of 8 request *channels* (RM0410 tables 26/27). On top of    */
/* the dma.h one-shot helpers, this API exposes what the stream       */
/* controller adds: double-buffer mode (two memory targets the        */
/* hardware alternates between), selectable item sizes and priority,  */
/* and per-stream interrupt callbacks.                                */
/*                                                                    */
/* Only available when building for STEFI_MCU_STM32F7.                */
/**********************************************************************/
#if !defined(STEFI_MCU_STM32F7)
#error "libstefi/dma_stream.h is the STM32F7 stream DMA; not available on this MCU family"
#endif

typedef enum {
    DMA_DIR_PERIPH_TO_MEM = 0,
    DMA_DIR_MEM_TO_PERIPH = 1,
    DMA_DIR_MEM_TO_MEM    = 2,   /* DMA2 only */
} dma_dir_t;

typedef enum {
    DMA_WIDTH_8  = 0,
    DMA_WIDTH_16 = 1,
    DMA_WIDTH_32 = 2,
} dma_width_t;

typedef enum {
    DMA_PRIORITY_LOW = 0,
    DMA_PRIORITY_MEDIUM,
    DMA_PRIORITY_HIGH,
    DMA_PRIORITY_VERY_HIGH,
} dma_priority_t;

/* Per-stream event flags, normalised to the stream-0 bit positions of
 * DMA_LISR. Passed to the stream callback. */
#define DMA_STREAM_FLAG_FE  (1u << 0)   /* FIFO error           */
#define DMA_STREAM_FLAG_DME (1u << 2)   /* direct-mode error    */
#define DMA_STREAM_FLAG_TE  (1u << 3)   /* transfer error       */
#define DMA_STREAM_FLAG_HT  (1u << 4)   /* half transfer        */
#define DMA_STREAM_FLAG_TC  (1u << 5)   /* transfer complete    */

typedef struct {
    peripheral_dma_t dma;      /* PERIPHERAL_DMA1 / PERIPHERAL_DMA2 */
    uint8_t stream;            /* 0..7 */
    uint8_t channel;           /* request channel 0..7 (CHSEL) */
    dma_dir_t dir;
    dma_width_t periph_width;
    dma_width_t mem_width;
    bool periph_inc;
    bool mem_inc;
    bool circular;             /* implied by double_buffer */
    bool double_buffer;        /* M0/M1 ping-pong; CT tells the active one */
    dma_priority_t priority;
    bool irq_tc;               /* raise the callback on transfer complete */
    bool irq_ht;               /* ... on half transfer */
    bool irq_te;               /* ... on transfer/direct-mode error */
} dma_stream_config_t;

/* Callback in interrupt context; flags = DMA_STREAM_FLAG_* (already cleared). */
typedef void (*dma_stream_callback_t)(void *aux, uint32_t flags);

/**
 * @brief Configure a stream (it is disabled first, flags cleared).
 *
 * @param periph  peripheral register address (PAR); for M2M: the source
 * @param mem0    memory target 0 (M0AR); for M2M: the destination
 * @param mem1    memory target 1 (M1AR), double-buffer mode only (else NULL)
 * @param items   number of items per target (NDTR, 1..65535)
 *
 * Enables the controller clock and, if any irq_* is set, the NVIC line.
 * The stream is left disabled; start it with dma_stream_enable().
 */
void dma_stream_configure(const dma_stream_config_t *cfg, volatile const void *periph,
                          void *mem0, void *mem1, uint16_t items);

void dma_stream_enable(peripheral_dma_t dma, uint8_t stream);
/** Disable and wait until the hardware has actually stopped the stream. */
void dma_stream_disable(peripheral_dma_t dma, uint8_t stream);
bool dma_stream_is_enabled(peripheral_dma_t dma, uint8_t stream);

/** Reload NDTR (stream must be disabled). */
void dma_stream_set_items(peripheral_dma_t dma, uint8_t stream, uint16_t items);
uint16_t dma_stream_get_items(peripheral_dma_t dma, uint8_t stream);

/**
 * @brief Re-point memory target 0 or 1. In double-buffer mode only the
 * target that is NOT currently active (see dma_stream_current_target) may
 * be changed while the stream runs.
 */
void dma_stream_set_memory(peripheral_dma_t dma, uint8_t stream, uint8_t target, void *addr);
/** @return 0 if the stream is currently writing M0AR, 1 for M1AR. */
uint8_t dma_stream_current_target(peripheral_dma_t dma, uint8_t stream);

void dma_stream_register_handler(peripheral_dma_t dma, uint8_t stream,
                                 dma_stream_callback_t fn, void *aux);

/** NVIC source of a stream (for interrupts_set_priority()). */
nvic_source_t dma_stream_irq(peripheral_dma_t dma, uint8_t stream);
