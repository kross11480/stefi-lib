/**********************************************************************/
/* STM32F7 system clock: 200 MHz from the 8 MHz ST-Link MCO (HSE      */
/* bypass), 48 MHz for USB from PLLSAI.                               */
/**********************************************************************/
#include "libstefi/system.h"
#include "libstefi/util.h"

#include "internal/rcc_f7.h"
#include "internal/pwr_flash_f7.h"

/*
 * Clock tree (identical to the CubeMX setup of the USB oscilloscope):
 *   HSE 8 MHz (bypass) / PLLM 4 = 2 MHz VCO input
 *   main PLL: x200 = 400 MHz VCO, /P 2 = 200 MHz SYSCLK = HCLK
 *   PLLSAI:   x96  = 192 MHz VCO, /P 4 = 48 MHz -> CK48 (USB OTG FS)
 *   APB1 = APB2 = HCLK/4 = 50 MHz, so the timer kernels run at 100 MHz.
 */
enum {
    PLL_M = 4,
    PLL_N = 200,
    PLL_P = 2,
    PLL_Q = 9,          /* PLLQ output unused (CK48 comes from PLLSAI) */
    PLLSAI_N = 96,
    PLLSAI_P_DIV4 = 1,  /* PLLSAIP field: 00 /2, 01 /4, 10 /6, 11 /8 */
    PLLSAI_Q = 4,
    PLLSAI_R = 2,
    APB_DIV4 = 5,       /* PPREx: 101 = /4 */
    FLASH_WS = 6,       /* 180 MHz < HCLK <= 210 MHz @ 2.7-3.6 V */
};

const uint32_t sys_clk = 200000000u;

void system_init(void)
{
    //1. power: regulator scale 1 (reset default) + over-drive for > 180 MHz
    RCC_F7->APB1ENR |= BIT(28);                     // PWREN
    (void) RCC_F7->APB1ENR;
    PWR_F7->CR1 |= (3u << 14);                      // VOS = scale 1

    //2. HSE in bypass mode (8 MHz MCO from the ST-Link)
    RCC_F7->CR |= BIT(RCC_CR_HSEBYP) | BIT(RCC_CR_HSEON);
    while (!(RCC_F7->CR & BIT(RCC_CR_HSERDY)));

    //3. main PLL
    RCC_F7->CR &= ~BIT(RCC_CR_PLLON);
    while (RCC_F7->CR & BIT(RCC_CR_PLLRDY));
    RCC_F7->PLLCFGR = (PLL_M << 0) | (PLL_N << 6) | (((PLL_P / 2) - 1) << 16)
                    | BIT(22)                       // PLLSRC = HSE
                    | (PLL_Q << 24) | (2u << 28);   // PLLR = 2 (unused)
    RCC_F7->CR |= BIT(RCC_CR_PLLON);
    while (!(RCC_F7->CR & BIT(RCC_CR_PLLRDY)));

    //4. over-drive (required above 180 MHz), RM0410 §4.1.4
    PWR_F7->CR1 |= BIT(16);                         // ODEN
    while (!(PWR_F7->CSR1 & BIT(16)));              // ODRDY
    PWR_F7->CR1 |= BIT(17);                         // ODSWEN
    while (!(PWR_F7->CSR1 & BIT(17)));              // ODSWRDY

    //5. flash wait states + ART accelerator / prefetch BEFORE speeding up
    FLASH_F7->ACR = (FLASH_F7->ACR & ~0xFu) | FLASH_WS | BIT(8) | BIT(9);
    while ((FLASH_F7->ACR & 0xFu) != FLASH_WS);

    //6. bus prescalers: AHB /1, APB1 /4, APB2 /4
    RCC_F7->CFGR = (RCC_F7->CFGR & ~((0xFu << 4) | (0x7u << 10) | (0x7u << 13)))
                 | (APB_DIV4 << 10) | (APB_DIV4 << 13);

    //7. switch SYSCLK to the PLL
    RCC_F7->CFGR = (RCC_F7->CFGR & ~0x3u) | 0x2u;
    while (((RCC_F7->CFGR >> 2) & 0x3u) != 0x2u);

    //8. PLLSAI -> 48 MHz for USB, select it as CK48
    RCC_F7->CR &= ~BIT(RCC_CR_PLLSAION);
    while (RCC_F7->CR & BIT(RCC_CR_PLLSAIRDY));
    RCC_F7->PLLSAICFGR = (PLLSAI_N << 6) | (PLLSAI_P_DIV4 << 16)
                       | (PLLSAI_Q << 24) | (PLLSAI_R << 28);
    RCC_F7->CR |= BIT(RCC_CR_PLLSAION);
    while (!(RCC_F7->CR & BIT(RCC_CR_PLLSAIRDY)));
    RCC_F7->DCKCFGR2 |= BIT(RCC_DCKCFGR2_CK48MSEL);
}

bool system_init_g4(uint32_t hz) { (void) hz; return false; }
uint32_t system_get_core_clock(void) { return sys_clk; }
