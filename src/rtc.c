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

#include <stddef.h>

#define RTC_BASE 0x40002800u
#define RTC_REG(off) (*(volatile uint32_t *) (RTC_BASE + (off)))
#define RTC_WUTR  RTC_REG(0x14)
#define RTC_WPR   RTC_REG(0x24)
/* L4: CR at 0x08, flags in ISR (0x0C): WUTWF [2], WUTF [10].
 * G4: CR at 0x18, WUTWF in ICSR (0x0C) [2], WUTF in SR (0x50) [2], clear in SCR (0x5C). */
#define RTC_CR    (*(mcu_is_g4() ? &RTC_REG(0x18) : &RTC_REG(0x08)))
#define RTC_ISR   RTC_REG(0x0C)
#define CR_WUTE   BIT(10)
#define CR_WUTIE  BIT(14)

#define RCC_CSR_REG (*(volatile uint32_t *) 0x40021094u)
#define EXTI_LINE_RTC_WAKEUP 20u
#define INTERRUPT_SOURCE_RTC_WKUP ((nvic_source_t) 3)

static struct {
    callbackfn_t fn;
    void *aux;
} wake;

void rtc_init_lsi(void)
{
    RCC_CSR_REG |= BIT(0);                       // LSION
    while (!(RCC_CSR_REG & BIT(1)));             // LSIRDY

    pwr_backup_access(true);
    uint32_t sel = (RCC->BDCR >> 8) & 0x3u;
    if (sel != 0x2u && sel != 0u) {              // other source: backup domain reset
        RCC->BDCR |= BIT(16);
        RCC->BDCR &= ~BIT(16);
    }
    RCC->BDCR = (RCC->BDCR & ~(0x3u << 8)) | (0x2u << 8) | BIT(15);   // RTCSEL = LSI, RTCEN
    if (mcu_is_g4()) RCC->APB1ENR1 |= BIT(10);  // RTCAPBEN (G4 only)
}

static void rtc_unlock(void) { RTC_WPR = 0xCA; RTC_WPR = 0x53; }
static void rtc_lock(void)   { RTC_WPR = 0xFF; }

static void rtc_clear_wakeup_flag(void)
{
    if (mcu_is_g4()) RTC_REG(0x5C) = BIT(2);    // SCR.CWUTF
    else RTC_ISR &= ~BIT(10);                    // ISR.WUTF (write 0)
    EXTI->PR1 = BIT(EXTI_LINE_RTC_WAKEUP);
}

void rtc_wakeup_stop(void)
{
    rtc_unlock();
    RTC_CR &= ~(CR_WUTE | CR_WUTIE);
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
    RTC_CR &= ~(CR_WUTE | CR_WUTIE);
    while (!(RTC_ISR & BIT(2)));                 // WUTWF: WUTR writable
    RTC_WUTR = count;
    RTC_CR = (RTC_CR & ~0x7u) | (uint32_t) clk;
    rtc_clear_wakeup_flag();
    RTC_CR |= CR_WUTIE | CR_WUTE;
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
