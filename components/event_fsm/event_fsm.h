#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "libstefi/ringbuffer.h"

/*
 * Event-driven finite state machine (one active state, entry/exit actions)
 * with an interrupt-safe event queue.
 *
 * A state is a function receiving every event; FSM_EV_ENTRY / FSM_EV_EXIT
 * are delivered on transitions. ISRs post events (fsm_post), the main loop
 * drains them (fsm_poll) — so all state logic runs in thread context:
 *
 *   enum { EV_START = FSM_EV_USER, EV_DONE };
 *   static void st_idle(fsm_t *f, fsm_event_t ev) {
 *       switch (ev) {
 *       case FSM_EV_ENTRY: led_on(LED_GREEN); break;
 *       case EV_START:     fsm_transition(f, st_busy); break;
 *       }
 *   }
 *   fsm_init(&fsm, queue_buf, sizeof queue_buf, &app);
 *   fsm_start(&fsm, st_idle);
 *   while (1) fsm_poll(&fsm);
 */

typedef uint8_t fsm_event_t;

#define FSM_EV_NONE  0u
#define FSM_EV_ENTRY 1u
#define FSM_EV_EXIT  2u
#define FSM_EV_USER  3u    /* first application event */

typedef struct fsm fsm_t;
typedef void (*fsm_state_t)(fsm_t *fsm, fsm_event_t event);

struct fsm {
    fsm_state_t state;
    rb_t queue;
    void *ctx;              /* application context, free for the states */
    volatile uint32_t dropped;  /* events lost to a full queue */
};

/** @param queue_buf storage for the queue (holds queue_size - 1 events) */
void fsm_init(fsm_t *fsm, uint8_t *queue_buf, uint16_t queue_size, void *ctx);

/** Enter the initial state (runs its FSM_EV_ENTRY). Also used for a reset. */
void fsm_start(fsm_t *fsm, fsm_state_t initial);

/** Exit the current state, enter @p next. Call from a state function. */
void fsm_transition(fsm_t *fsm, fsm_state_t next);

/** Deliver an event to the current state right now (thread context). */
void fsm_dispatch(fsm_t *fsm, fsm_event_t event);

/** Queue an event; safe from any interrupt. false if the queue was full. */
bool fsm_post(fsm_t *fsm, fsm_event_t event);

/** Take the next queued event without dispatching it. */
bool fsm_get(fsm_t *fsm, fsm_event_t *event);

/** Dispatch one queued event, if any. @return true if one was handled. */
bool fsm_poll(fsm_t *fsm);

/** Drop all queued events. */
void fsm_flush(fsm_t *fsm);

static inline bool fsm_in_state(const fsm_t *fsm, fsm_state_t state)
{
    return fsm->state == state;
}
