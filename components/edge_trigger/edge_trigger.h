#pragma once

#include <stdbool.h>
#include "libstefi/gpio.h"
#include "libstefi/interrupts.h"

/*
 * Edge trigger: a one-shot interrupt on the board's trigger input
 * (edge_trigger table in stefi_board.h — e.g. a comparator output).
 *
 *   edge_trigger_init();
 *   edge_trigger_set_edge(RISING_EDGE);
 *   edge_trigger_arm(on_trigger, ctx);   // fires once, then disarms itself
 *
 * The callback runs in interrupt context, so time-critical reactions (such
 * as starting an ADC capture DMA) happen with interrupt latency only. Edges
 * that occurred while the trigger was disarmed are ignored, and an edge is
 * only accepted if the pin still shows the new level when the interrupt
 * runs (rejects glitches shorter than the interrupt latency).
 */

void edge_trigger_init(void);

void edge_trigger_set_edge(edge_t edge);   /* RISING_EDGE or FALLING_EDGE */
edge_t edge_trigger_get_edge(void);

void edge_trigger_arm(callbackfn_t fn, void *aux);
void edge_trigger_disarm(void);
bool edge_trigger_is_armed(void);

/** NVIC priority of the trigger line (default 1: just below a capture DMA). */
void edge_trigger_set_priority(uint8_t priority);
