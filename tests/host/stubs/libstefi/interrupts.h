#pragma once
/* Host-test stub: shadows the real libstefi header (see tests/host/run.sh). */
#include <stdint.h>
typedef void (*callbackfn_t)(void *aux_data);
typedef int nvic_source_t;
#define INTERRUPT_SOURCE_OTG_FS 67
static inline void interrupts_set_priority(nvic_source_t s, uint8_t p){(void)s;(void)p;}
static inline void interrupts_global_disable(void){}
static inline void interrupts_global_enable(void){}
