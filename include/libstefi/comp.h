#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "libstefi/interrupts.h"

/**********************************************************************/
/* Comparators COMP1/COMP2 (STM32L4)                                  */
/*                                                                    */
/* Output routed to EXTI (COMP1: line 21, COMP2: line 22): interrupt  */
/* on rising/falling crossings, which also wakes from Stop 0/1/2.     */
/**********************************************************************/

typedef enum {
    COMP_INM_VREFINT_1_4 = 0, COMP_INM_VREFINT_1_2, COMP_INM_VREFINT_3_4, COMP_INM_VREFINT,
    COMP_INM_DAC1, COMP_INM_DAC2, COMP_INM_IO1, COMP_INM_IO2,
} comp_inm_t;

typedef enum { COMP_POWER_HIGH = 0, COMP_POWER_MEDIUM = 1, COMP_POWER_ULTRALOW = 3 } comp_power_t;

typedef struct {
    uint8_t instance;          /* 1 or 2 */
    uint8_t inp;               /* + input: COMP1 0 = PC5, 1 = PB2; COMP2 0 = PB4, 1 = PB6 */
    comp_inm_t inm;
    comp_power_t power;
    uint8_t hysteresis;        /* 0 none .. 3 high */
    bool invert;
} comp_config_t;

/** Pin to analog, comparator configured and enabled. */
void comp_init(const comp_config_t *cfg);
bool comp_output(uint8_t instance);
/** Interrupt (EXTI) on output edges; fn runs in interrupt context. */
void comp_enable_interrupt(uint8_t instance, bool rising, bool falling, callbackfn_t fn, void *aux);
