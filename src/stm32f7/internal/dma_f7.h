#pragma once
#include <stdint.h>

/**********************************************************************/
/* STM32F7 DMA (stream controller) — private register map (RM0410 §8) */
/**********************************************************************/
typedef struct {
    volatile uint32_t CR;      /* +0x00 stream configuration */
    volatile uint32_t NDTR;    /* +0x04 number of data items */
    volatile uint32_t PAR;     /* +0x08 peripheral address   */
    volatile uint32_t M0AR;    /* +0x0C memory 0 address     */
    volatile uint32_t M1AR;    /* +0x10 memory 1 address (double-buffer mode) */
    volatile uint32_t FCR;     /* +0x14 FIFO control         */
} DMA_Stream_F7_t;

typedef struct {
    volatile uint32_t LISR;    /* 0x00 streams 0..3 status */
    volatile uint32_t HISR;    /* 0x04 streams 4..7 status */
    volatile uint32_t LIFCR;   /* 0x08 streams 0..3 clear  */
    volatile uint32_t HIFCR;   /* 0x0C streams 4..7 clear  */
    DMA_Stream_F7_t S[8];      /* 0x10 + 0x18 * stream     */
} DMA_F7_t;

#define DMA1_F7 ((DMA_F7_t *) 0x40026000u)
#define DMA2_F7 ((DMA_F7_t *) 0x40026400u)

/* DMA_SxCR bits */
#define DMA_SxCR_EN       0u
#define DMA_SxCR_DMEIE    1u
#define DMA_SxCR_TEIE     2u
#define DMA_SxCR_HTIE     3u
#define DMA_SxCR_TCIE     4u
#define DMA_SxCR_DIR      6u    /* 2 bits: 00 P->M, 01 M->P, 10 M->M */
#define DMA_SxCR_CIRC     8u
#define DMA_SxCR_PINC     9u
#define DMA_SxCR_MINC    10u
#define DMA_SxCR_PSIZE   11u    /* 2 bits */
#define DMA_SxCR_MSIZE   13u    /* 2 bits */
#define DMA_SxCR_PL      16u    /* 2 bits */
#define DMA_SxCR_DBM     18u
#define DMA_SxCR_CT      19u
#define DMA_SxCR_CHSEL   25u    /* 4 bits on F76x (3 used) */

/* DMA_SxFCR */
#define DMA_SxFCR_DMDIS   2u

/* Bit offset of a stream's 6-bit flag group inside LISR/HISR. */
static inline uint32_t dma_f7_flag_shift(uint8_t stream)
{
    static const uint8_t shift[4] = {0u, 6u, 16u, 22u};
    return shift[stream & 3u];
}
