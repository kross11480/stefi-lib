/*
 * Acquisition state machine — same states, transitions and status strings
 * as the V1 firmware (Core/Src/fsm.c), on stefi's event_fsm component.
 *
 *   init --APP--> idle --RUN_START--> acquisition_start --(trigger, DMA done)-->
 *   acquisition_done --READY_FOR_DATA--> data_transmission --RECEIVED-->
 *   wait_for_ack --CONTINUE--> acquisition_start
 *                --STOP-----> idle
 *   RESET (any state) --> init
 */
#include "scope.h"

static uint8_t event_queue[64];
fsm_t scope_fsm;

static void st_idle(fsm_t *fsm, fsm_event_t ev);
static void st_acquisition_start(fsm_t *fsm, fsm_event_t ev);
static void st_acquisition_done(fsm_t *fsm, fsm_event_t ev);
static void st_data_transmission(fsm_t *fsm, fsm_event_t ev);
static void st_wait_for_ack(fsm_t *fsm, fsm_event_t ev);

void scope_fsm_init(void)
{
    fsm_init(&scope_fsm, event_queue, sizeof event_queue, &scope);
}

/* (Re)connect: start over with an empty queue. */
void scope_fsm_reset(void)
{
    fsm_flush(&scope_fsm);
    fsm_start(&scope_fsm, scope_state_init);
}

/* One main-loop step: handle one queued event. RESET works in any state and
 * must NOT flush the queue — the PC sends "RESET" and "APP" back to back. */
bool scope_fsm_step(void)
{
    fsm_event_t ev;
    if (!fsm_get(&scope_fsm, &ev)) return false;
    if (ev == EV_RESET) {
        fsm_start(&scope_fsm, scope_state_init);
    } else {
        fsm_dispatch(&scope_fsm, ev);
    }
    return true;
}

/* init: (re)initialise, wait for the PC application ("APP") */
void scope_state_init(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_acquisition_stop();
            scope_reset_config();
            scope_status_leds(1);
            break;
        case EV_START_APP:
            fsm_transition(fsm, st_idle);
            break;
        default:
            break;
    }
}

/* idle: configuration commands, wait for RUN_START */
static void st_idle(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_status_leds(2);
            scope_reply("STATE:IDLE");
            break;
        case EV_CONFIGURE_FREQ:
            scope_apply_sample_rate();
            scope_reply("SAMPLING_FREQUENCY: %lu", (unsigned long) scope.rate_hz);
            break;
        case EV_CONFIGURE_REF_VOLT:
            scope_apply_reference();
            scope_reply("REFERENCE_VOLTAGE: %lu", (unsigned long) scope.vref_mv);
            break;
        case EV_CONFIGURE_TRIG_RISE_EDGE:
        case EV_CONFIGURE_TRIG_FALL_EDGE:
            scope_apply_trigger_edge(ev == EV_CONFIGURE_TRIG_RISE_EDGE ? RISING_EDGE : FALLING_EDGE);
            scope_reply(scope.trigger_edge == RISING_EDGE ? "TRIGGER:RISING_EDGE"
                                                          : "TRIGGER:FALLING_EDGE");
            break;
        case EV_CONFIGURE_MOV_AVG:
            scope_apply_movavg();
            scope_reply("MOVING_AVERAGE: %lu", (unsigned long) scope.movavg_points);
            break;
        case EV_SAMPL_RUN_START:
            fsm_transition(fsm, st_acquisition_start);
            break;
        default:
            break;
    }
}

/* acquisition start: arm DMA + trigger, wait for the buffer to fill.
 * The user button B1 forces a capture without trigger condition. */
static void st_acquisition_start(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_status_leds(3);
            scope_reply("STATE:ACQUISITION_START");
            scope_acquisition_arm();
            break;
        case EV_B1_PRESSED:
            scope_manual_trigger();
            break;
        case EV_SAMPL_DONE:
            fsm_transition(fsm, st_acquisition_done);
            break;
        default:
            break;
    }
}

/* acquisition done: raw port words -> signed samples (+ filter) */
static void st_acquisition_done(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_status_leds(4);
            scope_reply("STATE:ACQUISITION_DONE");
            scope_acquisition_stop();
            scope_preprocess();
            break;
        case EV_SAMPL_RDY_FOR_DATA:
            fsm_transition(fsm, st_data_transmission);
            break;
        default:
            break;
    }
}

/* transmission: 2 bytes/sample little endian, then one overflow byte */
static void st_data_transmission(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_status_leds(5);
            scope_send_data();
            break;
        case EV_SAMPL_RECEIVED:
            fsm_transition(fsm, st_wait_for_ack);
            break;
        default:
            break;
    }
}

/* wait for the PC: another acquisition or back to idle */
static void st_wait_for_ack(fsm_t *fsm, fsm_event_t ev)
{
    switch (ev) {
        case FSM_EV_ENTRY:
            scope_status_leds(6);
            scope_reply("STATE:WAIT_FOR_ACK");
            break;
        case EV_SAMPL_ACK_CONTINUE:
            fsm_transition(fsm, st_acquisition_start);
            break;
        case EV_SAMPL_ACK_STOP:
            fsm_transition(fsm, st_idle);
            break;
        default:
            break;
    }
}
