#include "libstefi/iwdg.h"

#define IWDG_KR   (*(volatile uint32_t *) 0x40003000u)
#define IWDG_PR   (*(volatile uint32_t *) 0x40003004u)
#define IWDG_RLR  (*(volatile uint32_t *) 0x40003008u)
#define IWDG_SR   (*(volatile uint32_t *) 0x4000300Cu)   /* PVU [0], RVU [1], WVU [2] */
#define IWDG_WINR (*(volatile uint32_t *) 0x40003010u)

void iwdg_start(iwdg_prescaler_t prescaler, uint16_t reload, uint16_t window)
{
    IWDG_KR = 0xCCCC;               // start (also starts the LSI)
    IWDG_KR = 0x5555;               // unlock PR/RLR/WINR
    IWDG_PR = prescaler;
    IWDG_RLR = reload & 0x0FFFu;
    while (IWDG_SR & 0x3u);         // PVU/RVU: values taken over
    // RM: writing WINR reloads the counter; without a window, refresh instead
    if (window < IWDG_WINDOW_DISABLED) {
        IWDG_WINR = window & 0x0FFFu;
        while (IWDG_SR & 0x4u);
    } else {
        IWDG_KR = 0xAAAA;
    }
}

void iwdg_refresh(void)
{
    IWDG_KR = 0xAAAA;
}
