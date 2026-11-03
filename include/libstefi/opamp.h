#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* Operational amplifiers OPAMP1/OPAMP2 (STM32L4)                     */
/*   OPAMP1: VINP PA0 / DAC1 ch1, VINM PA1, VOUT PA3                  */
/*   OPAMP2: VINP PA6 / DAC1 ch2, VINM PA7, VOUT PB0                  */
/**********************************************************************/

typedef enum { OPAMP_MODE_STANDALONE = 0, OPAMP_MODE_PGA = 2, OPAMP_MODE_FOLLOWER = 3 } opamp_mode_t;

typedef struct {
    uint8_t instance;          /* 1 or 2 */
    opamp_mode_t mode;
    bool vinp_dac;             /* + input from the DAC channel instead of the pin */
    uint8_t pga_gain_log2;     /* PGA: 1 = x2, 2 = x4, 3 = x8, 4 = x16 */
    bool low_power;
} opamp_config_t;

/** Pins analog, configured and enabled. */
void opamp_init(const opamp_config_t *cfg);
/** Change the PGA gain while running. */
void opamp_set_gain(uint8_t instance, uint8_t pga_gain_log2);
void opamp_set_low_power(uint8_t instance, bool low_power);
void opamp_disable(uint8_t instance);
