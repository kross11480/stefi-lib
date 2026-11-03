/**********************************************************************/
/* OPAMP (STM32L4, RM0351 §24)                                        */
/**********************************************************************/
#include "libstefi/opamp.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/util.h"
#include "internal/rcc_internal.h"

#define OPAMP_CSR(n) (*(volatile uint32_t *) (0x40007800u + 0x10u * ((n) - 1u)))
#define CSR_OPAEN   BIT(0)
#define CSR_OPALPM  BIT(1)
#define CSR_RANGE   (1u << 31)   /* OPAMP1 CSR: VDDA > 2.4 V (applies to both) */

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

    OPAMP_CSR(1) |= CSR_RANGE;
    uint32_t pga = (cfg->pga_gain_log2 ? (uint32_t) (cfg->pga_gain_log2 - 1u) : 0u) & 3u;
    uint32_t csr = ((uint32_t) cfg->mode << 2) | (pga << 4)
                 | (cfg->mode == OPAMP_MODE_PGA ? (0x2u << 8) : 0u)   // VM_SEL: internal (PGA)
                 | (cfg->vinp_dac ? BIT(10) : 0u)
                 | (cfg->low_power ? CSR_OPALPM : 0u);
    OPAMP_CSR(cfg->instance) = (OPAMP_CSR(cfg->instance) & CSR_RANGE) | csr | CSR_OPAEN;
}

void opamp_set_gain(uint8_t instance, uint8_t pga_gain_log2)
{
    uint32_t pga = (pga_gain_log2 ? (uint32_t) (pga_gain_log2 - 1u) : 0u) & 3u;
    OPAMP_CSR(instance) = (OPAMP_CSR(instance) & ~(0x3u << 4)) | (pga << 4);
}

void opamp_set_low_power(uint8_t instance, bool low_power)
{
    if (low_power) OPAMP_CSR(instance) |= CSR_OPALPM; else OPAMP_CSR(instance) &= ~CSR_OPALPM;
}

void opamp_disable(uint8_t instance)
{
    OPAMP_CSR(instance) &= ~CSR_OPAEN;
}
