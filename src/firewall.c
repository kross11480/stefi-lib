/**********************************************************************/
/* FIREWALL (STM32L4, RM0351 §4)                                      */
/**********************************************************************/
#include "libstefi/firewall.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"
#include "internal/firewall_internal.h"
#include "internal/gpio_internal.h"


void firewall_enable(const firewall_config_t *cfg)
{
    RCC->APB2ENR |= BIT(7);                  // FWEN (firewall clock)
    (void) RCC->APB2ENR;
    // segment registers hold offsets in the respective memory, aligned
    FIREWALL->CSSA = cfg->code_start & 0x00FFFF00u;
    FIREWALL->CSL = cfg->code_len & 0x003FFF00u;
    FIREWALL->NVDSSA = cfg->nvdata_start & 0x00FFFF00u;
    FIREWALL->NVDSL = cfg->nvdata_len & 0x003FFF00u;
    FIREWALL->VDSSA = cfg->vdata_start & 0x0001FFC0u;
    FIREWALL->VDSL = cfg->vdata_len & 0x0001FFC0u;
    FIREWALL->CR = (cfg->vdata_shared ? BIT(1) : 0u) | (cfg->vdata_executable ? BIT(2) : 0u);
    SYSCFG->CFGR1 &= ~BIT(0);                 // FWDIS = 0: firewall on
}

void firewall_set_vdata_shared(bool shared)
{
    if (shared) FIREWALL->CR |= BIT(1); else FIREWALL->CR &= ~BIT(1);
}

void firewall_set_pre_arm(bool armed)
{
    if (armed) FIREWALL->CR |= BIT(0); else FIREWALL->CR &= ~BIT(0);
}

bool firewall_reset_occurred(void)
{
    return (RCC->CSR & BIT(24)) != 0;       // FWRSTF
}
