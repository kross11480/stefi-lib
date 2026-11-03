#include "libstefi/system.h"

#include <rcc_internal.h>
#include <libstefi/util.h>

#include "internal/cortexm4_internal.h"
#include "internal/pwr_internal.h"
#include "internal/mcu_id.h"

//L4: APBx Clock same as Sys Clock
enum {
    APB_PSC_1 = 1, //Default
    APB_PSC_2 = 4,
    APB_PSC_4 = 5,
    APB_PSC_8 = 6,
    APB_PSC_16 = 7,
};

enum {
    HSI16_CLK = 16,
    PLL_M = 1, //Div Factor(1) FOR PLL Input(1..8)
    PLL_N = 10, //Mul Factor(10) for VC0
    PLL_R = 2, //Div Factor(2) for PLL Out(2,4,6,8)
};
#define APB1_PSC APB_PSC_1
#define APB2_PSC APB_PSC_1

const uint32_t sys_clk = ((HSI16_CLK/ PLL_M) * PLL_N) / PLL_R;

static uint32_t core_clock_hz = 16000000u;   /* HSI16 (G4) / MSI 4 MHz (L4) at reset */


//Initialize to 80 Mhz using HSI:
void system_init() {
    SCB->CPACR |= (0XF << 20); //FPU Enable
    //1. Enable HSI (G4: Default, L4: MSI) and Voltage scaling (Range 1 for L4, G4) for high perf.
    RCC->APB1ENR1 |= BIT(28);
    PWR->CR1 |= BIT(9); //range 1
    while((PWR->SR2 & BIT(10)) == BIT(10)); // While Voltage is changing

    RCC->CR |= BIT(8); //(explicit for L4, HSI)
    while((RCC->CR & BIT(10)) != BIT(10)); //bit 10 is zero, while oscillator not ready

    //2. Configure Flash Latency (g4: 2WS, l4: 4 WS (5cycles) for 80 MHZ)
    FLASH->ACR &= ~(0x7); //0 WS
    FLASH->ACR |= BIT(2); //4 WS
    FLASH->ACR |= BIT(8) | BIT(9); // Enable Instruction Cache, Prefetch

    //3. Configure PLL
    RCC->PLLCFGR = 0;
    RCC->PLLCFGR |= (PLL_M - 1) << 4;               // Set PLL_M
    RCC->PLLCFGR |= PLL_N << 8;                     // Set PLL_N
    RCC->PLLCFGR |= (((PLL_R - 2)/2) & 3) << 25 ;   // Set PLL_R
    RCC->PLLCFGR |= 2 << 0;                         //HSI Clock as PLL Input

    //4. Enable PLL
    RCC->CR |= BIT(24); //enable pll clock output
    while((RCC->CR & BIT(25)) != BIT(25)); //while pll unlocked
    RCC->PLLCFGR |= BIT(24); //PLL output enable

    //5. Select PLL as System Clock
    RCC->CFGR |= ((RCC->CFGR & (~3)) | 3);
    while (((RCC->CFGR >> 2) & 0b11) != 0b11); //Wait until PLL used as system clock
    core_clock_hz = sys_clk * 1000000u;
}

uint32_t system_get_core_clock(void)
{
    return core_clock_hz;
}

#define PWR_CR5 (*(volatile uint32_t *) 0x40007080u)   /* G4 only */

bool system_init_g4(uint32_t hz)
{
    const uint32_t mhz = hz / 1000000u;
    if (!mcu_is_g4() || hz % 2000000u || mhz < 16u || mhz > 170u) return false;

    const bool boost = mhz > 150u;
    // flash wait states, RM0440 table 9 (range 1 boost / normal)
    uint32_t ws = boost ? (mhz + 33u) / 34u - 1u : (mhz + 29u) / 30u - 1u;

    RCC->APB1ENR1 |= BIT(28);                  // PWREN
    (void) RCC->APB1ENR1;

    // run from HSI16 while the PLL is (re)programmed
    RCC->CR |= BIT(8);                         // HSION
    while (!(RCC->CR & BIT(10)));
    RCC->CFGR = (RCC->CFGR & ~0x3u) | 0x1u;    // SW = HSI16
    while (((RCC->CFGR >> 2) & 0x3u) != 0x1u);
    RCC->CR &= ~BIT(24);                       // PLLOFF
    while (RCC->CR & BIT(25));

    // regulator: range 1, boost above 150 MHz (R1MODE = 0 means boost)
    if (boost) PWR_CR5 &= ~BIT(8); else PWR_CR5 |= BIT(8);

    // wait states first (increasing frequency), + prefetch / I- / D-cache
    FLASH->ACR = (FLASH->ACR & ~0xFu) | ws | BIT(8) | BIT(9) | BIT(10);
    while ((FLASH->ACR & 0xFu) != ws);

    // PLL: HSI16 / M 4 = 4 MHz, x N, / R 2
    RCC->PLLCFGR = 2u                          // PLLSRC = HSI16
                 | (3u << 4)                   // PLLM = 4
                 | ((mhz / 2u) << 8)           // PLLN
                 | (0u << 25)                  // PLLR = 2
                 | BIT(24);                    // PLLREN
    RCC->CR |= BIT(24);
    while (!(RCC->CR & BIT(25)));

    // RM0440: going above 80 MHz, pass through AHB /2 for >= 1 us
    RCC->CFGR = (RCC->CFGR & ~(0xFu << 4)) | (0x8u << 4);   // HPRE = /2
    RCC->CFGR = (RCC->CFGR & ~(0x7u << 8 | 0x7u << 11));    // PPRE1/2 = /1
    RCC->CFGR = (RCC->CFGR & ~0x3u) | 0x3u;    // SW = PLL
    while (((RCC->CFGR >> 2) & 0x3u) != 0x3u);
    for (volatile uint32_t i = 0; i < 200u; i++);            // > 1 us
    RCC->CFGR &= ~(0xFu << 4);                 // HPRE = /1

    core_clock_hz = hz;
    return true;
}

#define RCC_CSR (*(volatile uint32_t *) 0x40021094u)   /* L4 and G4 */

uint32_t system_reset_flags(void)
{
    return RCC_CSR & 0xFC000000u;
}

void system_clear_reset_flags(void)
{
    RCC_CSR |= BIT(23);                        // RMVF
}

_Noreturn void system_reset(void)
{
    __asm volatile ("dsb" ::: "memory");
    *(volatile uint32_t *) 0xE000ED0Cu = (0x5FAu << 16) | BIT(2);   // AIRCR SYSRESETREQ
    __asm volatile ("dsb" ::: "memory");
    for (;;);
}