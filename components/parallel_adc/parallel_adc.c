#include "parallel_adc.h"

#include "stefi_board.h"

#include "libstefi/gpio.h"
#include "libstefi/timer.h"

#include <stddef.h>

/*
 * DMA backend: the F7 stream controller (double-buffer chaining) or the
 * L4/G4 channel controller (single transfer). Everything else — pins,
 * sample clock, conversion — is shared.
 */
#if defined(STEFI_MCU_STM32F7)
#include "libstefi/dma_stream.h"
#define CAP_FLAG_TC          DMA_STREAM_FLAG_TC
#define CAP_FLAG_ERR         (DMA_STREAM_FLAG_TE | DMA_STREAM_FLAG_DME)
#define cap_dma_disable()    dma_stream_disable(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_enable()     dma_stream_enable(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_irq()        dma_stream_irq(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_register(fn) dma_stream_register_handler(parallel_adc.dma, parallel_adc.dma_channel, fn, NULL)
#define CAP_DOUBLE_BUFFER    1
#else
#include "libstefi/dma_channel.h"
#define CAP_FLAG_TC          DMA_CHANNEL_FLAG_TC
#define CAP_FLAG_ERR         DMA_CHANNEL_FLAG_TE
#define cap_dma_disable()    dma_channel_disable(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_enable()     dma_channel_enable(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_irq()        dma_channel_irq(parallel_adc.dma, parallel_adc.dma_channel)
#define cap_dma_register(fn) dma_channel_register_handler(parallel_adc.dma, parallel_adc.dma_channel, fn, NULL)
#define CAP_DOUBLE_BUFFER    0
#endif

#define MAX_DMA_ITEMS  65535u
#define MAX_SEGMENTS   64u

static struct {
    uint32_t rate_hz;
    uint16_t *buf;
    uint32_t seg_len;
    uint32_t segs;
    uint32_t next_seg;            /* next segment to hand to the stream */
    volatile uint32_t done_segs;
    volatile bool busy;
    volatile bool running;
    volatile bool error;
    parallel_adc_done_t done;
    void *aux;
} cap;

#if CAP_DOUBLE_BUFFER
/* Sink for the stream between the last segment's completion and the
 * interrupt disabling it (the double-buffer stream never stops by itself).
 * At 10 MSPS this covers 100 us of interrupt latency. */
#define SCRATCH_ITEMS  1024u
static uint16_t scratch[SCRATCH_ITEMS];
#endif

static inline uint16_t *segment(uint32_t i)
{
    return cap.buf + i * cap.seg_len;
}

static void finish(bool error)
{
    cap_dma_disable();
    cap.error = error;
    cap.running = false;
    cap.busy = false;
    if (cap.done) cap.done(cap.aux);
}

static void on_dma(void *aux, uint32_t flags)
{
    (void) aux;
    if (!cap.running) return;

    if (flags & CAP_FLAG_ERR) {
        finish(true);
        return;
    }
    if (!(flags & CAP_FLAG_TC)) return;

    if (++cap.done_segs >= cap.segs) {
        finish(false);
        return;
    }
#if CAP_DOUBLE_BUFFER
    // The target that just completed is the one the stream left (CT flipped):
    // give it the next segment, or the scratch sink once all are handed out.
    uint8_t finished = dma_stream_current_target(parallel_adc.dma, parallel_adc.dma_channel) ^ 1u;
    uint16_t *next = (cap.next_seg < cap.segs) ? segment(cap.next_seg++) : scratch;
    dma_stream_set_memory(parallel_adc.dma, parallel_adc.dma_channel, finished, next);
#endif
}

void parallel_adc_set_priority(uint8_t priority)
{
    interrupts_set_priority(cap_dma_irq(), priority);
}

void parallel_adc_init(void)
{
    const parallel_adc_config_t *c = &parallel_adc;

    // data + overflow inputs (pull-downs: defined level with the ADC unpowered)
    peripheral_gpio_enable(gpio_get_port_from_portpin(c->data_lsb_pin));
    for (uint32_t i = 0; i < c->data_bits; i++) {
        gpio_id_t pin = (gpio_id_t) (c->data_lsb_pin + i);
        gpio_set_mode(pin, MODER_INPUT);
        gpio_set_pupd(pin, PULL_DOWN);
    }
    gpio_set_mode(c->overflow_pin, MODER_INPUT);
    gpio_set_pupd(c->overflow_pin, PULL_DOWN);

    // sample clock: timer PWM out, 50 % duty, active low (the update event —
    // and with it the DMA read — sits mid-way between two clock edges)
    peripheral_gpio_enable(gpio_get_port_from_portpin(c->clock_pin));
    gpio_set_alternate_function(c->clock_pin, c->clock_af);
    gpio_set_output_speed(c->clock_pin, VERY_HIGH_S);
    gpio_set_mode(c->clock_pin, MODER_AF);

    timer_init(c->clock_timer);
    timer_set_mode_pwm(c->clock_timer, c->clock_channel);
    parallel_adc_set_sample_rate(c->max_sample_rate_hz);
    timer_cc_enable(c->clock_timer, c->clock_channel, false);
    timer_enable_dma_request(c->clock_timer, TIMER_DMA_UPDATE);
    timer_start(c->clock_timer);

    cap_dma_register(on_dma);
    parallel_adc_set_priority(0);
}

uint32_t parallel_adc_set_sample_rate(uint32_t hz)
{
    const parallel_adc_config_t *c = &parallel_adc;
    if (cap.busy && cap.rate_hz) return cap.rate_hz;

    if (hz > c->max_sample_rate_hz) hz = c->max_sample_rate_hz;
    if (hz < c->min_sample_rate_hz) hz = c->min_sample_rate_hz;

    uint32_t ticks = c->timer_clk_hz / hz;
    if (ticks < 2u) ticks = 2u;
    if (ticks > 65536u) ticks = 65536u;   // 16-bit timers (prescaler 1)

    timer_stop(c->clock_timer);
    timer_set_period(c->clock_timer, 1, ticks);
    timer_set_compare(c->clock_timer, c->clock_channel, ticks / 2u);
    timer_start(c->clock_timer);

    cap.rate_hz = c->timer_clk_hz / ticks;
    return cap.rate_hz;
}

uint32_t parallel_adc_get_sample_rate(void)
{
    return cap.rate_hz;
}

bool parallel_adc_prepare(uint16_t *buf, uint32_t n, parallel_adc_done_t done, void *aux)
{
    if (cap.busy || buf == NULL) return false;

#if CAP_DOUBLE_BUFFER
    uint32_t segs = 2;
    while (segs <= MAX_SEGMENTS && ((n % segs) != 0 || n / segs > MAX_DMA_ITEMS)) segs++;
    if (segs > MAX_SEGMENTS || n / segs == 0) return false;
#else
    if (n == 0 || n > MAX_DMA_ITEMS) return false;
    uint32_t segs = 1;
#endif

    cap.buf = buf;
    cap.segs = segs;
    cap.seg_len = n / segs;
    cap.next_seg = 2;
    cap.done_segs = 0;
    cap.error = false;
    cap.running = false;
    cap.done = done;
    cap.aux = aux;

    const parallel_adc_config_t *c = &parallel_adc;
    GPIO_typeDef *port = gpio_get_base_address(c->data_lsb_pin);
#if CAP_DOUBLE_BUFFER
    const dma_stream_config_t cfg = {
        .dma = c->dma,
        .stream = c->dma_channel,
        .channel = c->dma_request,
        .dir = DMA_DIR_PERIPH_TO_MEM,
        .periph_width = DMA_WIDTH_16,   // IDR low half-word = the port pins
        .mem_width = DMA_WIDTH_16,
        .mem_inc = true,
        .double_buffer = true,
        .priority = DMA_PRIORITY_VERY_HIGH,
        .irq_tc = true,
        .irq_te = true,
    };
    dma_stream_configure(&cfg, &port->IDR, segment(0), segment(1), (uint16_t) cap.seg_len);
#else
    const dma_channel_config_t cfg = {
        .dma = c->dma,
        .channel = c->dma_channel,
        .request = c->dma_request,
        .dir = DMA_DIR_PERIPH_TO_MEM,
        .periph_width = DMA_WIDTH_16,   // IDR low half-word = the port pins
        .mem_width = DMA_WIDTH_16,
        .mem_inc = true,
        .priority = DMA_PRIORITY_VERY_HIGH,
        .irq_tc = true,
        .irq_te = true,
    };
    dma_channel_configure(&cfg, &port->IDR, segment(0), (uint16_t) cap.seg_len);
#endif
    cap.busy = true;
    return true;
}

void parallel_adc_start(void)
{
    if (!cap.busy || cap.running) return;
    cap.running = true;
    cap_dma_enable();
}

bool parallel_adc_busy(void) { return cap.busy; }
bool parallel_adc_running(void) { return cap.running; }
bool parallel_adc_error(void) { return cap.error; }

void parallel_adc_abort(void)
{
    cap_dma_disable();
    cap.running = false;
    cap.busy = false;
}

bool parallel_adc_convert(const uint16_t *raw, int16_t *out, uint32_t n)
{
    const parallel_adc_config_t *c = &parallel_adc;
    const uint32_t shift = c->data_lsb_pin & 0xFu;
    const uint16_t mask = (uint16_t) ((1u << c->data_bits) - 1u);
    const uint16_t ovf_bit = (uint16_t) (1u << (c->overflow_pin & 0xFu));
    const uint32_t ext = 16u - c->data_bits;
    uint16_t ovf = 0;

    for (uint32_t i = 0; i < n; i++) {
        uint16_t w = raw[i];
        ovf |= w & ovf_bit;
        uint16_t v = (uint16_t) ((w >> shift) & mask);
        // 2's complement: move the sign bit to bit 15, arithmetic shift back
        out[i] = c->signed_output ? (int16_t) ((int16_t) (v << ext) >> ext) : (int16_t) v;
    }
    return ovf != 0;
}
