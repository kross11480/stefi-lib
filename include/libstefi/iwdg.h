#pragma once
#include <stdint.h>

/**********************************************************************/
/* Independent watchdog (STM32L4 / STM32G4), clocked by the LSI       */
/* (~32 kHz, +-10 % or worse: timeouts are approximate).              */
/*                                                                    */
/* Timeout = (reload + 1) * 4 * 2^prescaler / 32 kHz. In window mode  */
/* a refresh is only allowed once the counter has dropped below       */
/* `window` — refreshing earlier resets the MCU too.                  */
/**********************************************************************/

typedef enum {
    IWDG_PRESCALER_4 = 0, IWDG_PRESCALER_8, IWDG_PRESCALER_16, IWDG_PRESCALER_32,
    IWDG_PRESCALER_64, IWDG_PRESCALER_128, IWDG_PRESCALER_256,
} iwdg_prescaler_t;

#define IWDG_WINDOW_DISABLED 0x0FFFu

/** Start the watchdog (cannot be stopped again until reset). */
void iwdg_start(iwdg_prescaler_t prescaler, uint16_t reload, uint16_t window);
void iwdg_refresh(void);
