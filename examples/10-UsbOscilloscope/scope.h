#pragma once
/*
 * USB oscilloscope — application layer.
 *
 * Port of kross11480/usb-oscilloscope (firmware USB_Oszi_Firmware_V1) from
 * the ST HAL + USB middleware to stefi-lib. The wire protocol is unchanged,
 * so the PC software (src/python/software/software_v1) works as-is.
 *
 *   scope_protocol.c  USB CDC bytes -> lines -> commands -> FSM events
 *   scope_fsm.c       the acquisition state machine (same states as V1)
 *   scope_hw.c        what the states do with the hardware components
 */
#include <stdbool.h>
#include <stdint.h>

#include "event_fsm.h"
#include "libstefi/gpio.h"

/* One acquisition = 100 000 samples (200 000 bytes on the wire). */
#define SCOPE_NUM_SAMPLES        100000u

#define SCOPE_DEFAULT_RATE_HZ    10000000u
#define SCOPE_DEFAULT_VREF_MV    4095u
#define SCOPE_DEFAULT_MOVAVG     0u        /* filter off */
#define SCOPE_VREF_FULL_MV       4095u     /* Vref = 4.096 V - Vdac */

enum scope_event {
    EV_RESET = FSM_EV_USER,
    EV_B1_PRESSED,
    EV_START_APP,
    EV_CONFIGURE_FREQ,
    EV_CONFIGURE_REF_VOLT,
    EV_CONFIGURE_TRIG_RISE_EDGE,
    EV_CONFIGURE_TRIG_FALL_EDGE,
    EV_CONFIGURE_MOV_AVG,
    EV_SAMPL_RUN_START,
    EV_SAMPL_DONE,
    EV_SAMPL_RDY_FOR_DATA,
    EV_SAMPL_RECEIVED,
    EV_SAMPL_ACK_CONTINUE,
    EV_SAMPL_ACK_STOP,
};

typedef struct {
    /* requested by the PC, applied by the idle state */
    uint32_t req_rate_hz;
    uint32_t req_vref_mv;
    uint32_t req_movavg_points;

    /* active configuration */
    uint32_t rate_hz;
    uint32_t vref_mv;
    edge_t trigger_edge;
    uint32_t movavg_points;   /* 0 = filter off */

    /* last acquisition */
    bool overflow;
    bool filtered;
} scope_t;

extern scope_t scope;
extern fsm_t scope_fsm;

/* scope_hw.c */
void scope_hw_init(void);
void scope_reset_config(void);
void scope_status_leds(uint8_t pattern);   /* bit0 green, bit1 blue, bit2 red */
void scope_apply_sample_rate(void);
void scope_apply_reference(void);
void scope_apply_trigger_edge(edge_t edge);
void scope_apply_movavg(void);
void scope_acquisition_arm(void);
void scope_manual_trigger(void);
void scope_acquisition_stop(void);
void scope_preprocess(void);
void scope_send_data(void);

/* scope_fsm.c */
void scope_fsm_init(void);
void scope_fsm_reset(void);
bool scope_fsm_step(void);
void scope_state_init(fsm_t *fsm, fsm_event_t ev);

/* scope_protocol.c */
void scope_protocol_init(void);
void scope_protocol_poll(void);
void scope_reply(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
