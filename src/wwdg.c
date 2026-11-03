#include <libstefi/wwdg.h>
#include <libstefi/util.h>
#include "internal/wwdg_internal.h"
#include "internal/rcc_internal.h"
#include "internal/mcu_id.h"
#include "libstefi/interrupts.h"

#include <stddef.h>

wwdg_t *wwdg = WWDG_BASE;


void wwdg_set_mode(const wwdg_status_t mode)
{
    wwdg->CR |= (mode << 7);
    wwdg->CR &= (mode << 7);
}

void wwdg_feed_timer(const uint8_t value)
{
    uint8_t upperBound = wwdg->CFR & 0x3F;
    uint8_t lowerBound = 0xC0;
    assert(value <= upperBound);
    assert(value >= lowerBound);
    wwdg->CR = value | BIT(7);
}

void wwdg_set_upper_bound(const uint8_t value)
{
    assert(value >= 0x3F);
    assert(value <= 0xC0);
    wwdg->CFR = (wwdg->CFR | BIT(7) | BIT(8) | BIT(9)) | value;
}

void wwdg_clear_interrupt()
{
    wwdg->SR = 0;   // EWIF is rc_w0
}

void wwdg_set_interrupt_mode(const wwdg_status_t mode){
    wwdg->CFR |= (mode << 7);
    wwdg->CFR &= (mode << 7);
}

static void (*ewi_fn)(void);

static uint32_t wdgtb_bits(uint8_t prescaler_log2)
{
    // G4: WDGTB[2:0] at [13:11]; L4: WDGTB[1:0] at [8:7]
    if (mcu_is_g4()) return ((uint32_t) (prescaler_log2 & 7u)) << 11;
    return ((uint32_t) (prescaler_log2 & 3u)) << 7;
}

void wwdg_start(uint8_t prescaler_log2, uint8_t window, uint8_t counter)
{
    RCC->APB1ENR1 |= BIT(11);          // WWDGEN
    (void) RCC->APB1ENR1;
    wwdg->CFR = wdgtb_bits(prescaler_log2) | (window & 0x7Fu);
    wwdg->CR = BIT(7) | (counter & 0x7Fu);   // WDGA + T[6:0]
}

void wwdg_refresh(uint8_t counter)
{
    wwdg->CR = counter & 0x7Fu;        // WDGA stays set (write 0 has no effect)
}

uint32_t wwdg_tick_us(uint32_t pclk_hz, uint8_t prescaler_log2)
{
    return (uint32_t) ((4096ull << prescaler_log2) * 1000000ull / pclk_hz);
}

void wwdg_enable_early_wakeup(void (*fn)(void))
{
    ewi_fn = fn;
    wwdg->SR = 0;
    wwdg->CFR |= BIT(9);               // EWI
    interrupts_enable_source((nvic_source_t) 0);   // WWDG_IRQn
}

void WWDG_IRQHandler(void)
{
    wwdg->SR = 0;
    if (ewi_fn) ewi_fn();
}
