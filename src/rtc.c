/**********************************************************************/
/* RTC wakeup timer — STM32L4 (RTC v2) / STM32G4 (RTC v3 layout)      */
/**********************************************************************/
#include "libstefi/rtc.h"
#include "libstefi/pwr.h"
#include "libstefi/interrupts.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"
#include "internal/gpio_internal.h"
#include "internal/mcu_id.h"
#include "internal/rtc_internal.h"

#include <stddef.h>

/*
 * L4 and G4 share TR, DR, PRER, WUTR and WPR, and WUTWF (bit 2 of L4 ISR /
 * G4 ICSR at 0x0C); CR moves (L4 0x08, G4 0x18) and the wakeup flag lives in
 * L4 ISR [10] / G4 SR [2] (cleared through SCR).
 */
static inline volatile uint32_t *rtc_cr(void)
{
    return mcu_is_g4() ? &RTC_G4->CR : &RTC_L4->CR;
}
#define CR_WUTE   BIT(10)
#define CR_WUTIE  BIT(14)

#define EXTI_LINE_RTC_WAKEUP 20u
#define INTERRUPT_SOURCE_RTC_WKUP ((nvic_source_t) 3)

static struct {
    callbackfn_t fn;
    void *aux;
} wake;

void rtc_init_lsi(void)
{
    RCC->CSR |= BIT(0);                       // LSION
    while (!(RCC->CSR & BIT(1)));             // LSIRDY

    pwr_backup_access(true);
    uint32_t sel = (RCC->BDCR >> 8) & 0x3u;
    if (sel != 0x2u && sel != 0u) {              // other source: backup domain reset
        RCC->BDCR |= BIT(16);
        RCC->BDCR &= ~BIT(16);
    }
    RCC->BDCR = (RCC->BDCR & ~(0x3u << 8)) | (0x2u << 8) | BIT(15);   // RTCSEL = LSI, RTCEN
    if (mcu_is_g4()) RCC->APB1ENR1 |= BIT(10);  // RTCAPBEN (G4 only)
}

static void rtc_unlock(void) { RTC_L4->WPR = 0xCA; RTC_L4->WPR = 0x53; }
static void rtc_lock(void)   { RTC_L4->WPR = 0xFF; }

static void rtc_clear_wakeup_flag(void)
{
    if (mcu_is_g4()) RTC_G4->SCR = BIT(2);      // SCR.CWUTF
    else RTC_L4->ISR &= ~BIT(10);                    // ISR.WUTF (write 0)
    EXTI->PR1 = BIT(EXTI_LINE_RTC_WAKEUP);
}

void rtc_wakeup_stop(void)
{
    rtc_unlock();
    (*rtc_cr()) &= ~(CR_WUTE | CR_WUTIE);
    rtc_lock();
    EXTI->IMR1 &= ~BIT(EXTI_LINE_RTC_WAKEUP);
    interrupts_disable_source(INTERRUPT_SOURCE_RTC_WKUP);
}

void rtc_wakeup_start(rtc_wakeup_clock_t clk, uint16_t count, callbackfn_t fn, void *aux)
{
    wake.fn = fn;
    wake.aux = aux;
    pwr_backup_access(true);

    rtc_unlock();
    (*rtc_cr()) &= ~(CR_WUTE | CR_WUTIE);
    while (!(RTC_L4->ISR & BIT(2)));                 // WUTWF: WUTR writable
    RTC_L4->WUTR = count;
    (*rtc_cr()) = ((*rtc_cr()) & ~0x7u) | (uint32_t) clk;
    rtc_clear_wakeup_flag();
    (*rtc_cr()) |= CR_WUTIE | CR_WUTE;
    rtc_lock();

    // wakeup event -> EXTI line 20, rising edge, interrupt
    EXTI->RTSR1 |= BIT(EXTI_LINE_RTC_WAKEUP);
    EXTI->IMR1 |= BIT(EXTI_LINE_RTC_WAKEUP);
    interrupts_enable_source(INTERRUPT_SOURCE_RTC_WKUP);
}

void RTC_WKUP_IRQHandler(void)
{
    rtc_clear_wakeup_flag();
    if (wake.fn) wake.fn(wake.aux);
}

/* ------------- calendar / alarm / tamper / timestamp (L4 RTC v2) ------------- */

#define ISR_INIT   BIT(7)
#define ISR_INITF  BIT(6)
#define ISR_RSF    BIT(5)
#define ISR_ALRAWF BIT(0)
#define ISR_ALRAF  BIT(8)
#define ISR_TSF    BIT(11)
#define ISR_TSOVF  BIT(12)
#define ISR_TAMP1F BIT(13)

#define EXTI_LINE_RTC_ALARM 18u
#define EXTI_LINE_RTC_TAMP  19u

static struct { callbackfn_t fn; void *aux; } alarm_cb, tamper_cb, ts_cb;

static uint32_t bcd(uint8_t v)  { return (uint32_t) (((v / 10u) << 4) | (v % 10u)); }
static uint8_t unbcd(uint32_t v) { return (uint8_t) (((v >> 4) & 0xFu) * 10u + (v & 0xFu)); }

static void init_mode(bool on)
{
    if (on) {
        RTC_L4->ISR |= ISR_INIT;
        while (!(RTC_L4->ISR & ISR_INITF));
    } else {
        RTC_L4->ISR &= ~ISR_INIT;
    }
}

void rtc_set_prescaler(uint8_t async, uint16_t sync)
{
    pwr_backup_access(true);
    rtc_unlock();
    init_mode(true);
    RTC_L4->PRER = (uint32_t) sync & 0x7FFFu;
    RTC_L4->PRER |= (uint32_t) (async & 0x7Fu) << 16;
    init_mode(false);
    rtc_lock();
}

void rtc_set_datetime(const rtc_datetime_t *t)
{
    pwr_backup_access(true);
    rtc_unlock();
    init_mode(true);
    RTC_L4->TR = (bcd(t->hours) << 16) | (bcd(t->minutes) << 8) | bcd(t->seconds);
    RTC_L4->DR = (bcd(t->year) << 16) | ((uint32_t) (t->weekday & 7u) << 13)
           | (bcd(t->month) << 8) | bcd(t->day);
    init_mode(false);
    rtc_lock();
}

void rtc_get_datetime(rtc_datetime_t *t)
{
    RTC_L4->ISR &= ~ISR_RSF;                         // wait for fresh shadow registers
    while (!(RTC_L4->ISR & ISR_RSF));
    uint32_t tr = RTC_L4->TR, dr = RTC_L4->DR;           // TR read locks DR until read
    t->hours = unbcd((tr >> 16) & 0x3Fu);
    t->minutes = unbcd((tr >> 8) & 0x7Fu);
    t->seconds = unbcd(tr & 0x7Fu);
    t->year = unbcd((dr >> 16) & 0xFFu);
    t->weekday = (uint8_t) ((dr >> 13) & 7u);
    t->month = unbcd((dr >> 8) & 0x1Fu);
    t->day = unbcd(dr & 0x3Fu);
}

void rtc_alarm_a_set(uint8_t hours, uint8_t minutes, uint8_t seconds, callbackfn_t fn, void *aux)
{
    alarm_cb.fn = fn;
    alarm_cb.aux = aux;
    pwr_backup_access(true);
    rtc_unlock();
    (*rtc_cr()) &= ~(BIT(8) | BIT(12));               // ALRAE, ALRAIE off
    while (!(RTC_L4->ISR & ISR_ALRAWF));
    RTC_L4->ALRMAR = (1u << 31)                      // MSK4: date/weekday don't care
               | (bcd(hours) << 16) | (bcd(minutes) << 8) | bcd(seconds);
    RTC_L4->ISR &= ~ISR_ALRAF;
    (*rtc_cr()) |= BIT(8) | BIT(12);
    rtc_lock();
    EXTI->RTSR1 |= BIT(EXTI_LINE_RTC_ALARM);
    EXTI->IMR1 |= BIT(EXTI_LINE_RTC_ALARM);
    interrupts_enable_source((nvic_source_t) 41);   // RTC_Alarm
}

void rtc_alarm_a_disable(void)
{
    rtc_unlock();
    (*rtc_cr()) &= ~(BIT(8) | BIT(12));
    rtc_lock();
}

void RTC_Alarm_IRQHandler(void)
{
    if (RTC_L4->ISR & ISR_ALRAF) {
        RTC_L4->ISR &= ~ISR_ALRAF;
        if (alarm_cb.fn) alarm_cb.fn(alarm_cb.aux);
    }
    EXTI->PR1 = BIT(EXTI_LINE_RTC_ALARM);
}

void rtc_backup_write(uint8_t index, uint32_t value)
{
    pwr_backup_access(true);
    RTC_L4->BKP[index & 31u] = value;
}

uint32_t rtc_backup_read(uint8_t index)
{
    return RTC_L4->BKP[index & 31u];
}

static void tamp_stamp_irq_on(void)
{
    EXTI->RTSR1 |= BIT(EXTI_LINE_RTC_TAMP);
    EXTI->IMR1 |= BIT(EXTI_LINE_RTC_TAMP);
    interrupts_enable_source((nvic_source_t) 2);    // TAMP_STAMP
}

void rtc_tamper1_enable(bool falling, callbackfn_t fn, void *aux)
{
    tamper_cb.fn = fn;
    tamper_cb.aux = aux;
    pwr_backup_access(true);
    // TAMP1E [0], TAMP1TRG [1] (1 = falling/low), TAMP1IE [16]; edge mode,
    // no filter, backup registers erased on detection (TAMP1NOERASE = 0)
    RTC_L4->TAMPCR = (RTC_L4->TAMPCR & ~(BIT(0) | BIT(1) | BIT(16)))
               | (falling ? BIT(1) : 0u) | BIT(16) | BIT(0);
    RTC_L4->ISR &= ~ISR_TAMP1F;
    tamp_stamp_irq_on();
}

void rtc_tamper1_disable(void)
{
    RTC_L4->TAMPCR &= ~(BIT(0) | BIT(16));
}

void rtc_timestamp_enable(bool falling, callbackfn_t fn, void *aux)
{
    ts_cb.fn = fn;
    ts_cb.aux = aux;
    pwr_backup_access(true);
    rtc_unlock();
    (*rtc_cr()) &= ~(BIT(11) | BIT(15));
    (*rtc_cr()) = ((*rtc_cr()) & ~BIT(3)) | (falling ? BIT(3) : 0u);   // TSEDGE
    RTC_L4->ISR &= ~(ISR_TSF | ISR_TSOVF);
    (*rtc_cr()) |= BIT(11) | BIT(15);                 // TSE, TSIE
    rtc_lock();
    tamp_stamp_irq_on();
}

void rtc_timestamp_get(rtc_datetime_t *t)
{
    uint32_t tr = RTC_L4->TSTR, dr = RTC_L4->TSDR;
    t->hours = unbcd((tr >> 16) & 0x3Fu);
    t->minutes = unbcd((tr >> 8) & 0x7Fu);
    t->seconds = unbcd(tr & 0x7Fu);
    t->year = 0;
    t->weekday = (uint8_t) ((dr >> 13) & 7u);
    t->month = unbcd((dr >> 8) & 0x1Fu);
    t->day = unbcd(dr & 0x3Fu);
}

void TAMP_STAMP_IRQHandler(void)
{
    uint32_t isr = RTC_L4->ISR;
    if (isr & ISR_TAMP1F) {
        RTC_L4->ISR &= ~ISR_TAMP1F;
        if (tamper_cb.fn) tamper_cb.fn(tamper_cb.aux);
    }
    if (isr & ISR_TSF) {
        RTC_L4->ISR &= ~(ISR_TSF | ISR_TSOVF);
        if (ts_cb.fn) ts_cb.fn(ts_cb.aux);
    }
    EXTI->PR1 = BIT(EXTI_LINE_RTC_TAMP);
}
