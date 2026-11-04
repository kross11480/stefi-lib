#pragma once
#include <stdint.h>

/*
 * RTC. The L4 (RTC v2) and G4 (RTC v3) layouts differ from offset 0x08 on;
 * the driver picks one at run time (mcu_id.h).
 */
typedef struct {
    volatile uint32_t TR;        /*!< time,                          Address offset: 0x00 */
    volatile uint32_t DR;        /*!< date,                          Address offset: 0x04 */
    volatile uint32_t CR;        /*!< control,                       Address offset: 0x08 */
    volatile uint32_t ISR;       /*!< init and status,               Address offset: 0x0C */
    volatile uint32_t PRER;      /*!< prescaler,                     Address offset: 0x10 */
    volatile uint32_t WUTR;      /*!< wakeup timer,                  Address offset: 0x14 */
    uint32_t RESERVED;           /*!< Reserved,                      Address offset: 0x18 */
    volatile uint32_t ALRMAR;    /*!< alarm A,                       Address offset: 0x1C */
    volatile uint32_t ALRMBR;    /*!< alarm B,                       Address offset: 0x20 */
    volatile uint32_t WPR;       /*!< write protection,              Address offset: 0x24 */
    volatile uint32_t SSR;       /*!< sub second,                    Address offset: 0x28 */
    volatile uint32_t SHIFTR;    /*!< shift control,                 Address offset: 0x2C */
    volatile uint32_t TSTR;      /*!< timestamp time,                Address offset: 0x30 */
    volatile uint32_t TSDR;      /*!< timestamp date,                Address offset: 0x34 */
    volatile uint32_t TSSSR;     /*!< timestamp sub second,          Address offset: 0x38 */
    volatile uint32_t CALR;      /*!< calibration,                   Address offset: 0x3C */
    volatile uint32_t TAMPCR;    /*!< tamper configuration,          Address offset: 0x40 */
    volatile uint32_t ALRMASSR;  /*!< alarm A sub second,            Address offset: 0x44 */
    volatile uint32_t ALRMBSSR;  /*!< alarm B sub second,            Address offset: 0x48 */
    volatile uint32_t OR;        /*!< option,                        Address offset: 0x4C */
    volatile uint32_t BKP[32];   /*!< backup registers,              Address offset: 0x50 */
} RTC_L4_t;

typedef struct {
    volatile uint32_t TR;        /*!< time,                          Address offset: 0x00 */
    volatile uint32_t DR;        /*!< date,                          Address offset: 0x04 */
    volatile uint32_t SSR;       /*!< sub second,                    Address offset: 0x08 */
    volatile uint32_t ICSR;      /*!< init control and status,       Address offset: 0x0C */
    volatile uint32_t PRER;      /*!< prescaler,                     Address offset: 0x10 */
    volatile uint32_t WUTR;      /*!< wakeup timer,                  Address offset: 0x14 */
    volatile uint32_t CR;        /*!< control,                       Address offset: 0x18 */
    uint32_t RESERVED0[2];       /*!< Reserved,                      Address offset: 0x1C */
    volatile uint32_t WPR;       /*!< write protection,              Address offset: 0x24 */
    uint32_t RESERVED1[10];      /*!< CALR .. ALRMBSSR,              Address offset: 0x28 */
    volatile uint32_t SR;        /*!< status,                        Address offset: 0x50 */
    volatile uint32_t MISR;      /*!< masked interrupt status,       Address offset: 0x54 */
    uint32_t RESERVED2;          /*!< Reserved,                      Address offset: 0x58 */
    volatile uint32_t SCR;       /*!< status clear,                  Address offset: 0x5C */
} RTC_G4_t;

#define RTC_L4 ((RTC_L4_t *) 0x40002800)
#define RTC_G4 ((RTC_G4_t *) 0x40002800)
