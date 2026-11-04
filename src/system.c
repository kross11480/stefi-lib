#include "libstefi/system.h"

#include <rcc_internal.h>
#include <libstefi/util.h>

#include "internal/cortexm4_internal.h"
#include "internal/pwr_internal.h"
#include "internal/mcu_id.h"
#include "internal/gpio_internal.h"
#include "internal/timer_internal.h"

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

static uint32_t core_clock_hz;   /* 0 = reset clock: HSI16 (G4) / MSI 4 MHz (L4) */


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
    if (core_clock_hz == 0) return mcu_is_g4() ? 16000000u : 4000000u;
    return core_clock_hz;
}

/* ------------------------------- L4 ------------------------------- */
static const uint32_t msi_hz[12] = {
    100000, 200000, 400000, 800000, 1000000, 2000000,
    4000000, 8000000, 16000000, 24000000, 32000000, 48000000,
};

/* L4 range-1 flash wait states: 16 MHz per WS (RM0351 table 11) */
static void l4_flash_ws(uint32_t hz)
{
    uint32_t ws = (hz > 16000000u) ? (hz - 1u) / 16000000u : 0u;
    FLASH->ACR = (FLASH->ACR & ~0x7u) | ws | BIT(8) | BIT(9) | BIT(10);
    while ((FLASH->ACR & 0x7u) != ws);
}

static void l4_set_msi_range(uint8_t range)
{
    RCC->CR |= BIT(0);                                  // MSION
    while (!(RCC->CR & BIT(1)));                        // MSIRDY: range change allowed
    RCC->CR = (RCC->CR & ~(0xFu << 4)) | ((uint32_t) range << 4) | BIT(3);   // MSIRGSEL
    while (!(RCC->CR & BIT(1)));
}

static void l4_sysclk_to_msi(void)
{
    RCC->CFGR &= ~0x3u;                                 // SW = MSI
    while (((RCC->CFGR >> 2) & 0x3u) != 0u);
}

bool system_init_l4(system_clk_src_t src, uint32_t hz)
{
    const uint32_t mhz = hz / 1000000u;
    if (mcu_is_g4() || hz % 2000000u || mhz < 16u || mhz > 80u) return false;

    RCC->APB1ENR1 |= BIT(28);                           // PWREN
    PWR->CR1 = (PWR->CR1 & ~(0x3u << 9)) | (0x1u << 9); // VOS range 1
    while (PWR->SR2 & BIT(10));                         // VOSF

    // run from MSI 4 MHz while the PLL is reprogrammed
    l4_flash_ws(80000000u);                             // max WS while switching
    l4_set_msi_range(6);
    l4_sysclk_to_msi();
    RCC->CR &= ~BIT(24);                                // PLLOFF
    while (RCC->CR & BIT(25));

    uint32_t pllsrc, m;
    if (src == SYSTEM_CLK_HSI16) {
        RCC->CR |= BIT(8);
        while (!(RCC->CR & BIT(10)));
        pllsrc = 2u; m = 4u;
    } else {
        pllsrc = 1u; m = 1u;
    }
    RCC->PLLCFGR = pllsrc | ((m - 1u) << 4) | ((mhz / 2u) << 8) | (0u << 25) | BIT(24);
    RCC->CR |= BIT(24);
    while (!(RCC->CR & BIT(25)));
    RCC->CFGR &= ~((0xFu << 4) | (0x7u << 8) | (0x7u << 11));   // AHB/APB1/APB2 = /1
    RCC->CFGR = (RCC->CFGR & ~0x3u) | 0x3u;             // SW = PLL
    while (((RCC->CFGR >> 2) & 0x3u) != 0x3u);
    l4_flash_ws(hz);
    core_clock_hz = hz;
    return true;
}

bool system_init_l4_msi(uint8_t range)
{
    if (mcu_is_g4() || range > 11u) return false;
    uint32_t hz = msi_hz[range];
    if (hz > system_get_core_clock()) l4_flash_ws(hz); // more WS before speeding up
    l4_set_msi_range(range);
    l4_sysclk_to_msi();
    RCC->CR &= ~BIT(24);                                // PLL off
    l4_flash_ws(hz);
    core_clock_hz = hz;
    return true;
}

system_clk_src_t system_pll_source(void)
{
    return ((RCC->PLLCFGR & 0x3u) == 2u) ? SYSTEM_CLK_HSI16 : SYSTEM_CLK_MSI;
}

void system_mco(mco_source_t src, uint8_t div_log2)
{
    RCC->AHB2ENR |= BIT(0);                             // GPIOA
    GPIO_typeDef *pa = (GPIO_typeDef *) GPIO_BASE;
    pa->AFR[1] &= ~0xFu;                                // PA8 AF0 = MCO
    pa->OSPEEDR |= 3u << 16;
    pa->MODER = (pa->MODER & ~(3u << 16)) | (2u << 16);
    RCC->CFGR = (RCC->CFGR & ~((0xFu << 24) | (0x7u << 28)))
              | ((uint32_t) src << 24) | ((uint32_t) (div_log2 & 7u) << 28);
}


uint32_t system_measure_lsi_hz(void)
{
    RCC->CSR |= BIT(0);                                 // LSION
    while (!(RCC->CSR & BIT(1)));
    RCC->APB2ENR |= BIT(17);                            // TIM16EN
    (void) RCC->APB2ENR;
    TIM16->CR1 = 0;                                // CR1: stopped
    TIM16_L4_OR1 = 0x1u;                             // OR1 TI1_RMP = LSI (L4)
    TIM16->PSC = 0;                                // PSC
    TIM16->ARR = 0xFFFFu;                          // ARR
    TIM16->CCMR1 = 0x1u | (0x3u << 2);               // CCMR1: CC1S = TI1, IC1PSC = /8
    TIM16->CCER = 0x1u;                             // CCER: CC1E
    TIM16->CR1 = 0x1u;                             // CEN
    uint32_t cap[2];
    for (int i = 0; i < 3; i++) {                       // first capture is discarded
        TIM16->SR = 0;
        while (!(TIM16->SR & BIT(1)));            // CC1IF
        uint32_t c = TIM16->CCR1;
        if (i) cap[i - 1] = c;
    }
    TIM16->CR1 = 0;
    TIM16->CCER = 0;
    uint32_t diff = (cap[1] - cap[0]) & 0xFFFFu;        // 8 LSI periods in core clock ticks
    return diff ? (uint32_t) ((uint64_t) system_get_core_clock() * 8u / diff) : 0u;
}


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
    if (boost) PWR->CR5 &= ~BIT(8); else PWR->CR5 |= BIT(8);

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


uint32_t system_reset_flags(void)
{
    return RCC->CSR & 0xFC000000u;
}

void system_clear_reset_flags(void)
{
    RCC->CSR |= BIT(23);                        // RMVF
}

_Noreturn void system_reset(void)
{
    __asm volatile ("dsb" ::: "memory");
    SCB->AIRCR = (0x5FAu << 16) | BIT(2);           // SYSRESETREQ
    __asm volatile ("dsb" ::: "memory");
    for (;;);
}