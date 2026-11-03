#pragma once
#include <stdint.h>

/**********************************************************************/
/* DMA  (Direct Memory Access)  — private register map               */
/**********************************************************************/
typedef struct {
    volatile uint32_t CCR;     /*!< channel configuration,            offset chan+0x00 */
    volatile uint32_t CNDTR;   /*!< number of data to transfer,       offset chan+0x04 */
    volatile uint32_t CPAR;    /*!< peripheral address,               offset chan+0x08 */
    volatile uint32_t CMAR;    /*!< memory address,                   offset chan+0x0C */
} DMA_Channel_t;

typedef struct {
    volatile uint32_t ISR;     /*!< interrupt status register,        Address offset: 0x00 */
    volatile uint32_t IFCR;    /*!< interrupt flag clear register,    Address offset: 0x04 */
} DMA_t;

#define DMA1_BASE 0x40020000u
#define DMA2_BASE 0x40020400u

#define DMA1 ((DMA_t *) DMA1_BASE)
#define DMA2 ((DMA_t *) DMA2_BASE)

/* Channel pointer from a DMA base and a 1-based channel number. Accepts
 * either the integer base (DMA1_BASE) or the controller pointer (DMA1) —
 * the (uint32_t) cast forces byte arithmetic in both cases.
 * e.g. DMA_CHANNEL(DMA1, 1) -> &DMA1 channel 1 (CCR at 0x40020008). */
#define DMA_CHANNEL(dma_base, ch) \
    ((DMA_Channel_t *) ((uint32_t)(dma_base) + 0x08u + ((uint32_t)((ch) - 1u)) * 0x14u))

/*
 * DMAMUX1 (G4 only). One CCR per request line; the low byte (DMAREQ_ID)
 * selects which peripheral request drives that channel. DMAMUX channels
 * are 0-based and map to DMA channels in order: mux 0..7 -> DMA1 ch1..8,
 * mux 8..15 -> DMA2 ch1..8 (G47x; on the 6-channel G431 DMA2 starts at mux 6).
 * Selected at run time in dma.c (internal/mcu_id.h).
 */
typedef struct {
    volatile uint32_t CCR;     /*!< request line config (DMAREQ_ID[7:0] in bits 0..7) */
} DMAMUX_Channel_t;

#define DMAMUX1_BASE 0x40020800u
#define DMAMUX1_CHANNEL(mux_ch) ((DMAMUX_Channel_t *) (DMAMUX1_BASE + (uint32_t)(mux_ch) * 0x04u))