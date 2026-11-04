#pragma once
#include <stdint.h>

/* DAC1 (L4 RM0351 §19) */
typedef struct {
    volatile uint32_t CR;        /*!< control,                       Address offset: 0x00 */
    volatile uint32_t SWTRIGR;   /*!< software trigger,              Address offset: 0x04 */
    volatile uint32_t DHR12R1;   /*!< ch1 12-bit right aligned,      Address offset: 0x08 */
    volatile uint32_t DHR12L1;   /*!< ch1 12-bit left aligned,       Address offset: 0x0C */
    volatile uint32_t DHR8R1;    /*!< ch1 8-bit right aligned,       Address offset: 0x10 */
    volatile uint32_t DHR12R2;   /*!< ch2 12-bit right aligned,      Address offset: 0x14 */
    volatile uint32_t DHR12L2;   /*!< ch2 12-bit left aligned,       Address offset: 0x18 */
    volatile uint32_t DHR8R2;    /*!< ch2 8-bit right aligned,       Address offset: 0x1C */
    volatile uint32_t DHR12RD;   /*!< dual 12-bit right aligned,     Address offset: 0x20 */
    volatile uint32_t DHR12LD;   /*!< dual 12-bit left aligned,      Address offset: 0x24 */
    volatile uint32_t DHR8RD;    /*!< dual 8-bit right aligned,      Address offset: 0x28 */
    volatile uint32_t DOR1;      /*!< ch1 data output,               Address offset: 0x2C */
    volatile uint32_t DOR2;      /*!< ch2 data output,               Address offset: 0x30 */
    volatile uint32_t SR;        /*!< status,                        Address offset: 0x34 */
    volatile uint32_t CCR;       /*!< calibration control,           Address offset: 0x38 */
    volatile uint32_t MCR;       /*!< mode control,                  Address offset: 0x3C */
    volatile uint32_t SHSR1;     /*!< ch1 sample & hold sample time, Address offset: 0x40 */
    volatile uint32_t SHSR2;     /*!< ch2 sample & hold sample time, Address offset: 0x44 */
    volatile uint32_t SHHR;      /*!< sample & hold hold time,       Address offset: 0x48 */
    volatile uint32_t SHRR;      /*!< sample & hold refresh time,    Address offset: 0x4C */
} DAC_t;

#define DAC1 ((DAC_t *) 0x40007400)
