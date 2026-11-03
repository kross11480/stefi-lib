#pragma once
#include <stdbool.h>
#include <stdint.h>

/**********************************************************************/
/* Firewall (STM32L4)                                                 */
/*                                                                    */
/* Protects a code segment (flash), a non-volatile data segment      */
/* (flash) and a volatile data segment (SRAM1). Once enabled it stays */
/* on until reset. Code outside may only enter the protected code     */
/* through its call gate (segment start + 4); any other access to a   */
/* protected segment while the firewall is closed RESETS the MCU      */
/* (RCC_CSR.FWRSTF). The volatile segment may be shared (VDS: readable */
/* and writable from outside) and/or executable (VDE).               */
/* Granularity: 256 B (code / NV data), 64 B (volatile data).        */
/**********************************************************************/

typedef struct {
    uint32_t code_start, code_len;       /* flash; 0 = none */
    uint32_t nvdata_start, nvdata_len;   /* flash; 0 = none */
    uint32_t vdata_start, vdata_len;     /* SRAM1; 0 = none */
    bool vdata_shared;
    bool vdata_executable;
} firewall_config_t;

/** Program the segments and enable the firewall (irreversible until reset). */
void firewall_enable(const firewall_config_t *cfg);
/** VDS: let unprotected code access the volatile data segment (or not). */
void firewall_set_vdata_shared(bool shared);
/** FPA: let the protected code be called again after it left (set inside). */
void firewall_set_pre_arm(bool armed);
/** The last reset was caused by a firewall violation. */
bool firewall_reset_occurred(void);
