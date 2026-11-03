/**********************************************************************/
/* FIREWALL (STM32L4, RM0351 §4)                                      */
/**********************************************************************/
#include "libstefi/firewall.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"

#define FW_REG(off) (*(volatile uint32_t *) (0x40011C00u + (off)))
#define FW_CSSA   FW_REG(0x00)
#define FW_CSL    FW_REG(0x04)
#define FW_NVDSSA FW_REG(0x08)
#define FW_NVDSL  FW_REG(0x0C)
#define FW_VDSSA  FW_REG(0x10)
#define FW_VDSL   FW_REG(0x14)
#define FW_CR     FW_REG(0x20)
#define SYSCFG_CFGR1 (*(volatile uint32_t *) 0x40010004u)
#define RCC_CSR_R    (*(volatile uint32_t *) 0x40021094u)

void firewall_enable(const firewall_config_t *cfg)
{
    RCC->APB2ENR |= BIT(7);                  // FWEN (firewall clock)
    (void) RCC->APB2ENR;
    // segment registers hold offsets in the respective memory, aligned
    FW_CSSA = cfg->code_start & 0x00FFFF00u;
    FW_CSL = cfg->code_len & 0x003FFF00u;
    FW_NVDSSA = cfg->nvdata_start & 0x00FFFF00u;
    FW_NVDSL = cfg->nvdata_len & 0x003FFF00u;
    FW_VDSSA = cfg->vdata_start & 0x0001FFC0u;
    FW_VDSL = cfg->vdata_len & 0x0001FFC0u;
    FW_CR = (cfg->vdata_shared ? BIT(1) : 0u) | (cfg->vdata_executable ? BIT(2) : 0u);
    SYSCFG_CFGR1 &= ~BIT(0);                 // FWDIS = 0: firewall on
}

void firewall_set_vdata_shared(bool shared)
{
    if (shared) FW_CR |= BIT(1); else FW_CR &= ~BIT(1);
}

void firewall_set_pre_arm(bool armed)
{
    if (armed) FW_CR |= BIT(0); else FW_CR &= ~BIT(0);
}

bool firewall_reset_occurred(void)
{
    return (RCC_CSR_R & BIT(24)) != 0;       // FWRSTF
}
