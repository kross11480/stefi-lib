#pragma once
#include <stdint.h>

typedef enum {
    WWDG_DISABLED = 0, 
    WWDG_ENABLED = 1
} wwdg_status_t;

void wwdg_set_mode(const wwdg_status_t mode);
void wwdg_feed_timer(const uint8_t value);
void wwdg_clear_interrupt();
void wwdg_set_interrupt_mode(const wwdg_status_t mode);
void wwdg_set_upper_bound(const uint8_t value);


/**********************************************************************/
/* WWDG — full configuration (STM32L4 / STM32G4)                      */
/*                                                                    */
/* The 7-bit counter counts down from `counter` (0x40..0x7F) once per */
/* 4096 * 2^prescaler PCLK1 cycles; at 0x3F the MCU resets. Refreshing */
/* is only allowed once the counter is below `window`; refreshing     */
/* earlier resets as well.                                            */
/* Prescaler: L4 2^0..2^3, G4 2^0..2^7.                               */
/**********************************************************************/

/** Clock on, prescaler/window set, counter loaded, watchdog started. */
void wwdg_start(uint8_t prescaler_log2, uint8_t window, uint8_t counter);
/** Reload the counter (must be within the window). */
void wwdg_refresh(uint8_t counter);
/** Early-wakeup interrupt at counter 0x40 (last chance to refresh). */
void wwdg_enable_early_wakeup(void (*fn)(void));
/** Duration of one counter tick in microseconds, for the given PCLK1. */
uint32_t wwdg_tick_us(uint32_t pclk_hz, uint8_t prescaler_log2);
