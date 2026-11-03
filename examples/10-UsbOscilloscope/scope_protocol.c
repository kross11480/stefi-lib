/*
 * PC <-> scope protocol over USB CDC (unchanged from V1):
 *
 *   PC -> scope (newline terminated)      scope -> PC
 *   RESET                                 (re-enters init)
 *   APP                                   STATE:IDLE
 *   CONFIGURE_FREQ <Hz>                   SAMPLING_FREQUENCY: <Hz actually set>
 *   CONFIGURE_REF_VOLT <mV>               REFERENCE_VOLTAGE: <mV>
 *   CONFIGURE_TRIG_RISE_EDGE              TRIGGER:RISING_EDGE
 *   CONFIGURE_TRIG_FALL_EDGE              TRIGGER:FALLING_EDGE
 *   CONFIGURE_MOV_AVG <points, odd; 0=off> MOVING_AVERAGE: <points active>
 *   RUN_START                             STATE:ACQUISITION_START, later
 *                                         STATE:ACQUISITION_DONE
 *   READY_FOR_DATA                        200000 bytes int16 LE + 1 byte
 *                                         overflow (0xFF / 0x00)
 *   RECEIVED                              STATE:WAIT_FOR_ACK
 *   CONTINUE | STOP                       STATE:ACQUISITION_START | STATE:IDLE
 *   anything else                         ERR:UNKNOWN_CMD
 */
#include "scope.h"

#include <stdarg.h>
#include <stdio.h>

#include "cmd_line.h"
#include "libstefi/interrupts.h"
#include "libstefi/ringbuffer.h"
#include "libstefi/usb_cdc.h"
#include "libstefi/util.h"

/* USB interrupt -> main loop byte queue (the USB ISR must not block). */
static uint8_t rx_storage[1024];
static rb_t rx_queue;
static volatile uint32_t rx_dropped;

static char line_buf[128];
static cmd_line_t line_in;

static void on_usb_rx(const uint8_t *data, uint32_t len, void *aux)
{
    (void) aux;
    for (uint32_t i = 0; i < len; i++) {
        if (rb_is_full(&rx_queue)) { rx_dropped++; continue; }
        rb_insert(&rx_queue, data[i]);
    }
}

void scope_reply(const char *fmt, ...)
{
    char buf[96];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf - 1, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int) sizeof buf - 2) n = sizeof buf - 2;
    buf[n++] = '\n';
    usb_cdc_write(buf, (uint32_t) n);
}

static void post(fsm_event_t ev) { fsm_post(&scope_fsm, ev); }

static void cmd_reset(const char *a, void *x)    { (void) a; (void) x; post(EV_RESET); }
static void cmd_app(const char *a, void *x)      { (void) a; (void) x; post(EV_START_APP); }
static void cmd_rise(const char *a, void *x)     { (void) a; (void) x; post(EV_CONFIGURE_TRIG_RISE_EDGE); }
static void cmd_fall(const char *a, void *x)     { (void) a; (void) x; post(EV_CONFIGURE_TRIG_FALL_EDGE); }
static void cmd_run(const char *a, void *x)      { (void) a; (void) x; post(EV_SAMPL_RUN_START); }
static void cmd_ready(const char *a, void *x)    { (void) a; (void) x; post(EV_SAMPL_RDY_FOR_DATA); }
static void cmd_received(const char *a, void *x) { (void) a; (void) x; post(EV_SAMPL_RECEIVED); }
static void cmd_continue(const char *a, void *x) { (void) a; (void) x; post(EV_SAMPL_ACK_CONTINUE); }
static void cmd_stop(const char *a, void *x)     { (void) a; (void) x; post(EV_SAMPL_ACK_STOP); }

/* Commands with an argument: like V1, a missing/invalid number keeps the
 * previous request, the idle state then re-applies and reports it. */
static void cmd_freq(const char *args, void *x)
{
    (void) x;
    cmd_parse_u32(args, &scope.req_rate_hz);
    post(EV_CONFIGURE_FREQ);
}

static void cmd_vref(const char *args, void *x)
{
    (void) x;
    cmd_parse_u32(args, &scope.req_vref_mv);
    post(EV_CONFIGURE_REF_VOLT);
}

static void cmd_movavg(const char *args, void *x)
{
    (void) x;
    cmd_parse_u32(args, &scope.req_movavg_points);
    post(EV_CONFIGURE_MOV_AVG);
}

static const cmd_entry_t commands[] = {
    {"RESET",                    cmd_reset},
    {"APP",                      cmd_app},
    {"CONFIGURE_FREQ",           cmd_freq},
    {"CONFIGURE_REF_VOLT",       cmd_vref},
    {"CONFIGURE_TRIG_RISE_EDGE", cmd_rise},
    {"CONFIGURE_TRIG_FALL_EDGE", cmd_fall},
    {"CONFIGURE_MOV_AVG",        cmd_movavg},
    {"RUN_START",                cmd_run},
    {"READY_FOR_DATA",           cmd_ready},
    {"RECEIVED",                 cmd_received},
    {"CONTINUE",                 cmd_continue},
    {"STOP",                     cmd_stop},
};

static void on_line(const char *line, void *aux)
{
    if (!cmd_dispatch(commands, countof(commands), line, aux)) {
        scope_reply("ERR:UNKNOWN_CMD");
    }
}

void scope_protocol_init(void)
{
    rb_init(&rx_queue, rx_storage, sizeof rx_storage);
    cmd_line_init(&line_in, line_buf, sizeof line_buf, on_line, NULL);
    usb_cdc_register_rx_handler(on_usb_rx, NULL);
    usb_cdc_init(NULL);   // 0483:5740 "virtual COM port", like the ST stack
    // below the capture DMA (0) and the trigger (1): never delay a capture
    interrupts_set_priority(INTERRUPT_SOURCE_OTG_FS, 6);
}

/* Drain received bytes into the line assembler (main loop context). */
void scope_protocol_poll(void)
{
    uint8_t chunk[32];
    uint32_t n = 0;

    interrupts_global_disable();
    while (n < sizeof chunk && !rb_is_empty(&rx_queue)) {
        chunk[n++] = rb_remove(&rx_queue);
    }
    interrupts_global_enable();

    if (n) cmd_line_feed(&line_in, chunk, n);
}
