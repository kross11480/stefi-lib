/**********************************************************************/
/* PWR low-power modes — STM32L4 / STM32G4 (identical bits used here) */
/**********************************************************************/
#include "libstefi/pwr.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"

#define PWR_CR1  (*(volatile uint32_t *) 0x40007000u)   /* LPMS [2:0], DBP [8] */
#define PWR_CR3  (*(volatile uint32_t *) 0x40007008u)   /* EIWUL/EIWF [15] */
#define PWR_SR1  (*(volatile uint32_t *) 0x40007010u)   /* WUFx [4:0], SBF [8] */
#define PWR_SCR  (*(volatile uint32_t *) 0x40007018u)   /* CWUFx [4:0], CSBF [8] */
#define PWR_CR4  (*(volatile uint32_t *) 0x4000700Cu)   /* WPx polarity [4:0] */
#define PWR_SR2  (*(volatile uint32_t *) 0x40007014u)   /* REGLPF [9] */
#define SCB_SCR  (*(volatile uint32_t *) 0xE000ED10u)   /* SLEEPDEEP [2] */
#define SYST_CSR (*(volatile uint32_t *) 0xE000E010u)

enum { LPMS_STOP0 = 0u, LPMS_STOP1 = 1u, LPMS_STOP2 = 2u, LPMS_STANDBY = 3u, LPMS_SHUTDOWN = 4u };

static void pwr_clock(void)
{
    RCC->APB1ENR1 |= BIT(28);   // PWREN
    (void) RCC->APB1ENR1;
}

void pwr_backup_access(bool enable)
{
    pwr_clock();
    if (enable) PWR_CR1 |= BIT(8); else PWR_CR1 &= ~BIT(8);
}

static void enter_deep(uint32_t lpms)
{
    pwr_clock();
    PWR_CR1 = (PWR_CR1 & ~0x7u) | lpms;
    SCB_SCR |= BIT(2);                      // SLEEPDEEP
    __asm volatile ("dsb\n wfi\n isb" ::: "memory");
    SCB_SCR &= ~BIT(2);
}

static void enter_stop(uint32_t lpms)
{
    uint32_t systick = SYST_CSR & 0x3u;     // ENABLE | TICKINT
    SYST_CSR &= ~0x3u;
    enter_deep(lpms);
    SYST_CSR |= systick;
}

void pwr_enter_stop0(void) { enter_stop(LPMS_STOP0); }
void pwr_enter_stop1(void) { enter_stop(LPMS_STOP1); }

_Noreturn void pwr_enter_standby(void)
{
    pwr_clock();
    PWR_CR3 |= BIT(15);                     // internal wakeup line (RTC) enabled
    PWR_SCR = 0x1Fu;                        // clear stale WUFx, else no Standby
    SYST_CSR &= ~0x3u;
    for (;;) enter_deep(LPMS_STANDBY);
}

bool pwr_woke_from_standby(void)
{
    pwr_clock();
    return (PWR_SR1 & BIT(8)) != 0;
}

void pwr_clear_standby_flag(void)
{
    pwr_clock();
    PWR_SCR = BIT(8);                       // CSBF
}

void pwr_enter_sleep(void)
{
    SCB_SCR &= ~BIT(2);
    __asm volatile ("dsb\n wfi\n isb" ::: "memory");
}

void pwr_enter_stop2(void) { enter_stop(LPMS_STOP2); }

_Noreturn void pwr_enter_shutdown(void)
{
    pwr_clock();
    PWR_SCR = 0x1Fu;
    SYST_CSR &= ~0x3u;
    for (;;) enter_deep(LPMS_SHUTDOWN);
}

void pwr_lprun_enter(void)
{
    pwr_clock();
    PWR_CR1 |= BIT(14);                     // LPR
}

void pwr_lprun_exit(void)
{
    PWR_CR1 &= ~BIT(14);
    while (PWR_SR2 & BIT(9));               // REGLPF: main regulator ready
}

void pwr_enter_lp_sleep(void)
{
    pwr_lprun_enter();
    pwr_enter_sleep();
}

void pwr_enable_wakeup_pin(uint8_t n, bool falling)
{
    pwr_clock();
    uint32_t bit = 1u << (n - 1u);
    if (falling) PWR_CR4 |= bit; else PWR_CR4 &= ~bit;
    PWR_SCR = bit;                          // stale flag would wake at once
    PWR_CR3 |= bit;                         // EWUPn
}

void pwr_disable_wakeup_pin(uint8_t n)
{
    PWR_CR3 &= ~(1u << (n - 1u));
}

void pwr_clear_wakeup_flags(void)
{
    pwr_clock();
    PWR_SCR = 0x1Fu;
}

void pwr_set_voltage_range(uint8_t range)
{
    pwr_clock();
    PWR_CR1 = (PWR_CR1 & ~(0x3u << 9)) | ((uint32_t) (range == 2 ? 2u : 1u) << 9);
    while (PWR_SR2 & BIT(10));              // VOSF
}

void pwr_retain_sram2_in_standby(bool retain)
{
    pwr_clock();
    if (retain) PWR_CR3 |= BIT(8); else PWR_CR3 &= ~BIT(8);
}
