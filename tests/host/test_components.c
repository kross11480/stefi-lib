#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "cmd_line.h"
#include "dsp.h"

static char got[8][64]; static int ng;
static void on_line(const char *l, void *a){ (void)a; strcpy(got[ng++], l); }
static char last_args[64]; static int which;
static void f1(const char *a, void *x){ (void)x; which=1; strcpy(last_args,a);}
static void f2(const char *a, void *x){ (void)x; which=2; strcpy(last_args,a);}

/* V1 reference implementations (app.c) */
static int16_t v1_convert(uint16_t raw, int *of){
    uint16_t t = raw >> 3; if (t & (1<<12)) *of = 1; t &= 0xFFF;
    if ((t >> 11) & 1) { t -= 1; t ^= 0xFFF; return -(int16_t)t; } return t;
}
int main(void){
    char buf[16]; cmd_line_t cl;
    cmd_line_init(&cl, buf, sizeof buf, on_line, NULL);
    const char *in = "RUN_START\r\n\nAPP\nTHIS_LINE_IS_WAY_TOO_LONG\nCONT";
    cmd_line_feed(&cl, (const uint8_t*)in, strlen(in));
    cmd_line_feed(&cl, (const uint8_t*)"INUE\n", 5);
    assert(ng==3 && !strcmp(got[0],"RUN_START") && !strcmp(got[1],"APP") && !strcmp(got[2],"CONTINUE"));
    cmd_entry_t t[] = {{"CONFIGURE_FREQ", f1}, {"CONFIGURE_FREQX", f2}};
    assert(cmd_dispatch(t,2,"CONFIGURE_FREQ   1000000",NULL) && which==1 && !strcmp(last_args,"1000000"));
    assert(cmd_dispatch(t,2,"CONFIGURE_FREQX",NULL) && which==2 && !strcmp(last_args,""));
    assert(!cmd_dispatch(t,2,"CONFIGURE_FRE 1",NULL));
    uint32_t v=7; assert(!cmd_parse_u32("",&v) && v==7); assert(!cmd_parse_u32("12a",&v));
    assert(cmd_parse_u32("4294967295",&v) && v==4294967295u); assert(!cmd_parse_u32("4294967296",&v));

    /* moving average vs. V1 recursive filter (interior samples) */
    enum {N=100000}; static int16_t x[N], y[N], ref[N];
    srand(1); for(int i=0;i<N;i++) x[i]=(int16_t)((rand()%4096)-2048);
    for (unsigned w=1; w<=101; w+=2){
        assert(dsp_moving_average_i16(x,y,N,w));
        int32_t half=(w-1)/2, acc=0; memset(ref,0,sizeof ref);
        for(uint32_t i=0;i<=w-1;i++) acc+=x[i];
        ref[half]=(int16_t)(acc/(int32_t)w);
        for(uint32_t i=half+1;i<=(uint32_t)(N-(half+1));i++){ acc+=(x[i+half]-x[i-(half+1)]); ref[i]=(int16_t)(acc/(int32_t)w);}
        for(int i=half;i<=N-(half+1);i++) assert(y[i]==ref[i]);
        for(int i=0;i<half;i++) assert(y[i]==x[i] && y[N-1-i]==x[N-1-i]);
    }
    assert(!dsp_moving_average_i16(x,y,N,2) && !dsp_moving_average_i16(x,y,N,0) && !dsp_moving_average_i16(x,y,5,7));

    /* in-place variant == out-of-place, for many windows and lengths */
    static int16_t z[N];
    unsigned ws[] = {1,3,5,51,101,513,1025};
    uint32_t ns[] = {N, 1025, 2000, 77};
    for (unsigned a=0; a<sizeof ws/sizeof ws[0]; a++) for (unsigned b=0; b<4; b++) {
        uint32_t n=ns[b], w=ws[a];
        memcpy(z,x,n*sizeof z[0]);
        bool ok1 = dsp_moving_average_i16(x,y,n,w), ok2 = dsp_moving_average_i16_inplace(z,n,w);
        assert(ok1==ok2);
        if (ok1) assert(memcmp(y,z,n*sizeof z[0])==0);
    }
    assert(!dsp_moving_average_i16_inplace(z,N,1027));

    /* parallel_adc_convert logic (same expression as the component) vs V1 */
    for (uint32_t raw=0; raw<0x10000; raw++){
        int of1=0; int16_t r = v1_convert(raw,&of1);
        uint16_t v=(uint16_t)((raw>>3)&0xFFF); int16_t mine=(int16_t)((int16_t)(v<<4)>>4);
        int of2 = (raw & (1u<<15))!=0;
        assert(r==mine); assert(of1==of2);
    }
    puts("host tests: all passed");
}
