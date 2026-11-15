/* Host replay of the PC software (usb_oszi_pc_fsm.py) against the real
 * scope_fsm.c / scope_protocol.c; hardware actions are stubbed. */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "scope.h"
#include "libstefi/usb_cdc.h"

scope_t scope;
static usb_cdc_rx_callback_t rx_fn;
static char out[4096]; static size_t out_len; static uint32_t data_bytes;
static int armed, stopped;

void usb_cdc_init(const void *c){(void)c;}
void usb_cdc_register_rx_handler(usb_cdc_rx_callback_t f, void *a){(void)a; rx_fn=f;}
bool usb_cdc_dtr(void){return true;}
int32_t usb_cdc_write(const void *b, uint32_t n){
    if (n > 100) { data_bytes += n; return n; }
    if (n == 1 && data_bytes) { data_bytes += 1; return 1; }   /* overflow byte */
    memcpy(out+out_len,b,n); out_len+=n; out[out_len]=0; return n;
}
void scope_hw_init(void){}
void scope_reset_config(void){ scope.req_rate_hz=10000000; scope.rate_hz=10000000; scope.vref_mv=4095; scope.trigger_edge=RISING_EDGE; scope.movavg_points=0; }
void scope_status_leds(uint8_t p){(void)p;}
void scope_apply_sample_rate(void){ uint32_t t=100000000/scope.req_rate_hz; scope.rate_hz=100000000/t; }
void scope_apply_reference(void){ scope.vref_mv = scope.req_vref_mv>4095?4095:scope.req_vref_mv; }
void scope_apply_trigger_edge(edge_t e){ scope.trigger_edge=e; }
void scope_apply_movavg(void){ uint32_t p=scope.req_movavg_points; scope.movavg_points=(p==0||p%2==0)?0:p; }
void scope_acquisition_arm(void){ armed++; }
void scope_manual_trigger(void){}
void scope_acquisition_stop(void){ stopped++; }
void scope_preprocess(void){}
void scope_send_data(void){ usb_cdc_write("x", 200000); uint8_t o=0; usb_cdc_write(&o,1); }

static void run(void){ do { scope_protocol_poll(); } while (scope_fsm_step()); }
static void pc(const char *cmd){ rx_fn((const uint8_t*)cmd, strlen(cmd), NULL); for(int i=0;i<4;i++) run(); }
static void expect(const char *s){ if(!strstr(out,s)){ printf("MISSING [%s] in:\n[%s]\n",s,out); fflush(stdout); assert(0);} }
static void clear(void){ out_len=0; out[0]=0; }

int main(void){
    scope_fsm_init(); scope_protocol_init(); scope_fsm_reset();
    pc("RESET\nAPP\n");                         expect("STATE:IDLE\n"); clear();
    pc("CONFIGURE_FREQ 3000000\n");             expect("SAMPLING_FREQUENCY: 3030303\n"); clear();
    pc("CONFIGURE_REF_VOLT 2000\n");            expect("REFERENCE_VOLTAGE: 2000\n"); clear();
    pc("CONFIGURE_TRIG_FALL_EDGE\n");           expect("TRIGGER:FALLING_EDGE\n"); clear();
    pc("CONFIGURE_TRIG_RISE_EDGE\n");           expect("TRIGGER:RISING_EDGE\n"); clear();
    pc("CONFIGURE_MOV_AVG 51\n");               expect("MOVING_AVERAGE: 51\n"); clear();
    pc("CONFIGURE_MOV_AVG 50\n");               expect("MOVING_AVERAGE: 0\n"); clear();
    pc("BOGUS\n");                              expect("ERR:UNKNOWN_CMD\n"); clear();
    pc("GET_NUM_SAMPLES\n");                    expect("NUM_SAMPLES: 32768\n"); clear();   /* host = non-F7 */
    for (int round=0; round<2; round++) {
        pc(round ? "CONTINUE\n" : "RUN_START\n"); expect("STATE:ACQUISITION_START\n"); assert(armed==round+1); clear();
        pc("READY_FOR_DATA\n"); assert(data_bytes==0);   /* ignored before the capture is done */
        fsm_post(&scope_fsm, EV_SAMPL_DONE); run(); expect("STATE:ACQUISITION_DONE\n"); clear();
        pc("READY_FOR_DATA\n"); assert(data_bytes==200001); data_bytes=0;
        pc("RECEIVED\n");                         expect("STATE:WAIT_FOR_ACK\n"); clear();
    }
    pc("STOP\n");                               expect("STATE:IDLE\n"); clear();
    pc("RUN_START\n"); clear(); pc("RESET\n");  assert(!strstr(out,"STATE")); /* back in init */
    pc("RUN_START\n"); assert(out_len==0);      /* init ignores it */
    pc("APP\r\n");                              expect("STATE:IDLE\n");
    printf("protocol/FSM replay: all passed (%d stops)\n", stopped);
}
