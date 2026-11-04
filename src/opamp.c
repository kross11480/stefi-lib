/**********************************************************************/
/* OPAMP (STM32L4, RM0351 §24)                                        */
/**********************************************************************/
#include "libstefi/opamp.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"
#include "internal/opamp_internal.h"

#define CSR_OPAEN   BIT(0)
#define CSR_OPALPM  BIT(1)
#define CSR_RANGE   (1u << 31)   /* OPAMP1 CSR: VDDA > 2.4 V (applies to both) */

static inline OPAMP_t *opamp_regs(uint8_t n) { return (n == 2) ? OPAMP2 : OPAMP1; }

void opamp_init(const opamp_config_t *cfg)
{
    RCC->APB1ENR1 |= (1u << 30);                     // OPAMPEN
    (void) RCC->APB1ENR1;
    const gpio_id_t vinp = (cfg->instance == 1) ? A0 : A6;
    const gpio_id_t vinm = (cfg->instance == 1) ? A1 : A7;
    const gpio_id_t vout = (cfg->instance == 1) ? A3 : B0;
    peripheral_gpio_enable(gpio_get_port_from_portpin(vinp));
    peripheral_gpio_enable(gpio_get_port_from_portpin(vout));
    if (!cfg->vinp_dac) gpio_set_mode(vinp, MODER_ANALOG);
    if (cfg->mode == OPAMP_MODE_STANDALONE) gpio_set_mode(vinm, MODER_ANALOG);
    gpio_set_mode(vout, MODER_ANALOG);

    opamp_regs(1)->CSR |= CSR_RANGE;
    uint32_t pga = (cfg->pga_gain_log2 ? (uint32_t) (cfg->pga_gain_log2 - 1u) : 0u) & 3u;
    uint32_t csr = ((uint32_t) cfg->mode << 2) | (pga << 4)
                 | (cfg->mode == OPAMP_MODE_PGA ? (0x2u << 8) : 0u)   // VM_SEL: internal (PGA)
                 | (cfg->vinp_dac ? BIT(10) : 0u)
                 | (cfg->low_power ? CSR_OPALPM : 0u);
    opamp_regs(cfg->instance)->CSR = (opamp_regs(cfg->instance)->CSR & CSR_RANGE) | csr | CSR_OPAEN;
}

void opamp_set_gain(uint8_t instance, uint8_t pga_gain_log2)
{
    uint32_t pga = (pga_gain_log2 ? (uint32_t) (pga_gain_log2 - 1u) : 0u) & 3u;
    opamp_regs(instance)->CSR = (opamp_regs(instance)->CSR & ~(0x3u << 4)) | (pga << 4);
}

void opamp_set_low_power(uint8_t instance, bool low_power)
{
    if (low_power) opamp_regs(instance)->CSR |= CSR_OPALPM; else opamp_regs(instance)->CSR &= ~CSR_OPALPM;
}

void opamp_disable(uint8_t instance)
{
    opamp_regs(instance)->CSR &= ~CSR_OPAEN;
}
