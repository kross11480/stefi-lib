#pragma once
#include <stdint.h>
#include "libstefi/interrupts.h"

/**********************************************************************/
/* RTC wakeup timer (STM32L4 / STM32G4), clocked from the LSI         */
/* (~32 kHz). Wakes the MCU from Sleep/Stop (EXTI line 20) and from   */
/* Standby (internal wakeup line).                                    */
/**********************************************************************/

typedef enum {
    RTC_WAKEUP_CLK_DIV16 = 0,   /* LSI/16 ~ 2 kHz: up to 32.7 s, 0.5 ms steps */
    RTC_WAKEUP_CLK_DIV8  = 1,
    RTC_WAKEUP_CLK_DIV4  = 2,
    RTC_WAKEUP_CLK_DIV2  = 3,
    RTC_WAKEUP_CLK_1HZ   = 4,   /* ck_spre (~1 Hz with the reset prescalers) */
} rtc_wakeup_clock_t;

/** LSI on, RTC clocked from it and enabled (backup domain reset if it was
 *  running from another source). */
void rtc_init_lsi(void);

/**
 * @brief Start the periodic wakeup timer: an event every (count + 1) ticks.
 * @param fn  optional callback from the RTC_WKUP interrupt (may be NULL:
 *            the interrupt still runs, ending a Stop/WFI)
 */
void rtc_wakeup_start(rtc_wakeup_clock_t clk, uint16_t count, callbackfn_t fn, void *aux);
void rtc_wakeup_stop(void);

/* ---------------- calendar, alarm, tamper, timestamp (STM32L4) ---------------- */
#include <stdbool.h>

typedef struct {
    uint8_t hours, minutes, seconds;        /* 24 h */
    uint8_t year, month, day, weekday;      /* year 0..99, weekday 1 = Monday .. 7 */
} rtc_datetime_t;

/** Prescalers: ck_spre = rtcclk / ((async + 1) * (sync + 1)) should be 1 Hz,
 *  e.g. LSI 32 kHz: async 127, sync 249; LSE 32768: 127, 255. */
void rtc_set_prescaler(uint8_t async, uint16_t sync);
void rtc_set_datetime(const rtc_datetime_t *t);
void rtc_get_datetime(rtc_datetime_t *t);

/** Alarm A at hh:mm:ss every day (date ignored); callback from the RTC_Alarm IRQ. */
void rtc_alarm_a_set(uint8_t hours, uint8_t minutes, uint8_t seconds, callbackfn_t fn, void *aux);
void rtc_alarm_a_disable(void);

/** Backup registers BKP0R..BKP31R: survive reset and Standby (VBAT domain). */
void rtc_backup_write(uint8_t index, uint32_t value);
uint32_t rtc_backup_read(uint8_t index);

/** Tamper 1 (PC13): an edge erases the backup registers and calls fn
 *  (RTC_TAMP_STAMP IRQ). falling = active on the falling edge. */
void rtc_tamper1_enable(bool falling, callbackfn_t fn, void *aux);
void rtc_tamper1_disable(void);

/** Timestamp on the TS pin (PC13): the calendar is latched on the edge. */
void rtc_timestamp_enable(bool falling, callbackfn_t fn, void *aux);
void rtc_timestamp_get(rtc_datetime_t *t);   /* time, day, month, weekday (no year) */
