#include "libstefi/iwdg.h"
#include "internal/iwdg_internal.h"


void iwdg_start(iwdg_prescaler_t prescaler, uint16_t reload, uint16_t window)
{
    IWDG->KR = 0xCCCC;               // start (also starts the LSI)
    IWDG->KR = 0x5555;               // unlock PR/RLR/WINR
    IWDG->PR = prescaler;
    IWDG->RLR = reload & 0x0FFFu;
    while (IWDG->SR & 0x3u);         // PVU/RVU: values taken over
    // RM: writing WINR reloads the counter; without a window, refresh instead
    if (window < IWDG_WINDOW_DISABLED) {
        IWDG->WINR = window & 0x0FFFu;
        while (IWDG->SR & 0x4u);
    } else {
        IWDG->KR = 0xAAAA;
    }
}

void iwdg_refresh(void)
{
    IWDG->KR = 0xAAAA;
}
