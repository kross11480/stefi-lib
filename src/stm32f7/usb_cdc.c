/**********************************************************************/
/* USB CDC-ACM device on the STM32F7 OTG_FS core (slave / FIFO mode)  */
/**********************************************************************/
#include "libstefi/usb_cdc.h"
#include "libstefi/gpio.h"
#include "libstefi/peripheral.h"
#include "libstefi/interrupts.h"
#include "libstefi/util.h"
#include "internal/usb_otg_f7.h"

#include <stddef.h>

#define EP0_MPS        64u
#define DATA_MPS       64u
#define NOTIF_MPS      8u
#define EP_DATA        1u      /* 0x01 OUT / 0x81 IN */
#define EP_NOTIF       2u      /* 0x82 IN */
#define NUM_EPS        6u      /* OTG_FS: EP0..EP5 */

/* FIFO RAM: 320 words (1.25 KB) total on OTG_FS. */
#define RX_FIFO_WORDS    128u
#define TX0_FIFO_WORDS    32u  /* >= largest EP0 reply (67 B config desc) */
#define TX1_FIFO_WORDS   128u  /* 8 bulk packets in flight */
#define TX2_FIFO_WORDS    16u

#define IN_CHUNK  (64u * DATA_MPS)   /* bytes per programmed IN transfer */

/* standard requests */
#define REQ_GET_STATUS        0x00u
#define REQ_CLEAR_FEATURE     0x01u
#define REQ_SET_FEATURE       0x03u
#define REQ_SET_ADDRESS       0x05u
#define REQ_GET_DESCRIPTOR    0x06u
#define REQ_GET_CONFIGURATION 0x08u
#define REQ_SET_CONFIGURATION 0x09u
#define REQ_GET_INTERFACE     0x0Au
#define REQ_SET_INTERFACE     0x0Bu
/* CDC class requests */
#define CDC_SET_LINE_CODING        0x20u
#define CDC_GET_LINE_CODING        0x21u
#define CDC_SET_CONTROL_LINE_STATE 0x22u
#define CDC_SEND_BREAK             0x23u

#define DESC_DEVICE        1u
#define DESC_CONFIGURATION 2u
#define DESC_STRING        3u

typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_t;

static struct {
    usb_cdc_rx_callback_t rx_fn;
    void *rx_aux;
    usb_cdc_line_state_callback_t ls_fn;
    void *ls_aux;
    volatile bool configured;
    volatile bool dtr;
    volatile bool rts;
    volatile uint32_t dtr_drops;   /* epoch: a blocked write aborts when it changes */
    uint8_t config_value;
    union { usb_setup_t req; uint32_t words[2]; } setup;
    uint8_t ep0_out[EP0_MPS];
    uint32_t ep0_out_len;
    bool ep0_out_pending;          /* a control OUT data stage is expected */
    uint8_t line_coding[7];
    uint8_t rx_packet[DATA_MPS];
    uint8_t desc_buf[2 + 2 * 30];  /* UTF-16 string descriptor scratch */
} usb = {
    .line_coding = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8},   /* 115200 8N1 */
};

static usb_cdc_config_t ident = {
    .vid = 0x0483, .pid = 0x5740,
    .manufacturer = "stefi-lib",
    .product = "STefi CDC Virtual ComPort",
    .serial = "000000000001",
};

/**************************** descriptors ****************************/
static uint8_t device_desc[18] = {
    18, DESC_DEVICE,
    0x00, 0x02,           /* bcdUSB 2.00 */
    0x02, 0x00, 0x00,     /* class CDC (communications), per-interface sub/proto */
    EP0_MPS,
    0, 0, 0, 0,           /* idVendor / idProduct: filled from ident */
    0x00, 0x02,           /* bcdDevice 2.00 */
    1, 2, 3,              /* iManufacturer, iProduct, iSerialNumber */
    1                     /* bNumConfigurations */
};

#define CONFIG_DESC_LEN 67u
static const uint8_t config_desc[CONFIG_DESC_LEN] = {
    /* configuration */
    9, DESC_CONFIGURATION, CONFIG_DESC_LEN, 0, 2, 1, 0, 0x80, 50,   /* bus powered, 100 mA */
    /* interface 0: CDC communication, ACM */
    9, 4, 0, 0, 1, 0x02, 0x02, 0x01, 0,
    5, 0x24, 0x00, 0x10, 0x01,               /* header functional, CDC 1.10 */
    5, 0x24, 0x01, 0x00, 1,                  /* call management: data on interface 1 */
    4, 0x24, 0x02, 0x02,                     /* ACM: line coding + control line state */
    5, 0x24, 0x06, 0, 1,                     /* union: master 0, slave 1 */
    7, 5, 0x80 | EP_NOTIF, 0x03, NOTIF_MPS, 0, 16,   /* interrupt IN */
    /* interface 1: CDC data */
    9, 4, 1, 0, 2, 0x0A, 0x00, 0x00, 0,
    7, 5, EP_DATA, 0x02, DATA_MPS, 0, 0,             /* bulk OUT */
    7, 5, 0x80 | EP_DATA, 0x02, DATA_MPS, 0, 0,      /* bulk IN */
};

static const uint8_t lang_desc[4] = {4, DESC_STRING, 0x09, 0x04};   /* en-US */

static uint32_t string_desc(const char *s)
{
    uint32_t n = 0;
    /* <= 30 chars: a 64-byte (31-char) descriptor would need a ZLP when the
     * host asks for more, which ep0_send() does not generate */
    while (s[n] && n < 30u) {
        usb.desc_buf[2 + 2 * n] = (uint8_t) s[n];
        usb.desc_buf[3 + 2 * n] = 0;
        n++;
    }
    usb.desc_buf[0] = (uint8_t) (2 + 2 * n);
    usb.desc_buf[1] = DESC_STRING;
    return 2 + 2 * n;
}

/**************************** core helpers ****************************/
static void flush_tx_fifo(uint32_t num)   /* num 0..15, 0x10 = all */
{
    OTG_GRSTCTL = GRSTCTL_TXFFLSH | (num << 6);
    while (OTG_GRSTCTL & GRSTCTL_TXFFLSH);
}

static void flush_rx_fifo(void)
{
    OTG_GRSTCTL = GRSTCTL_RXFFLSH;
    while (OTG_GRSTCTL & GRSTCTL_RXFFLSH);
}

static void fifo_write(uint8_t ep, const uint8_t *src, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t w = 0;
        for (uint32_t b = 0; b < 4 && (i + b) < len; b++) {
            w |= (uint32_t) src[i + b] << (8 * b);
        }
        OTG_FIFO(ep) = w;
    }
}

static void fifo_read(uint8_t *dst, uint32_t len)
{
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t w = OTG_FIFO(0);
        for (uint32_t b = 0; b < 4 && (i + b) < len; b++) {
            if (dst) dst[i + b] = (uint8_t) (w >> (8 * b));
        }
    }
}

static void ep0_arm_out(void)
{
    OTG_DOEPTSIZ(0) = DOEPTSIZ0_STUPCNT_3 | (1u << EPTSIZ_PKTCNT_POS) | EP0_MPS;
    OTG_DOEPCTL(0) |= EPCTL_EPENA | EPCTL_CNAK;
}

/* Data (or status, len = 0) stage on EP0 IN; len <= 127 (2 packets + rest). */
static void ep0_send(const uint8_t *data, uint32_t len, uint16_t w_length)
{
    if (len > w_length) len = w_length;
    uint32_t pkts = (len == 0) ? 1u : (len + EP0_MPS - 1u) / EP0_MPS;
    OTG_DIEPTSIZ(0) = (pkts << EPTSIZ_PKTCNT_POS) | len;
    OTG_DIEPCTL(0) |= EPCTL_EPENA | EPCTL_CNAK;
    fifo_write(0, data, len);
}

static void ep0_status_in(void)
{
    ep0_send(NULL, 0, 0);
}

static void ep0_stall(void)
{
    OTG_DIEPCTL(0) |= EPCTL_STALL;
    OTG_DOEPCTL(0) |= EPCTL_STALL;   /* both cleared by the core on the next SETUP */
}

static void ep1_arm_out(void)
{
    OTG_DOEPTSIZ(EP_DATA) = (1u << EPTSIZ_PKTCNT_POS) | DATA_MPS;
    OTG_DOEPCTL(EP_DATA) |= EPCTL_EPENA | EPCTL_CNAK;
}

static void set_line_state(bool dtr, bool rts)
{
    if (usb.dtr && !dtr) usb.dtr_drops++;
    usb.dtr = dtr;
    usb.rts = rts;
    if (usb.ls_fn) usb.ls_fn(dtr, rts, usb.ls_aux);
}

static void configure_endpoints(uint8_t value)
{
    usb.config_value = value;
    if (value == 0) {
        OTG_DIEPCTL(EP_DATA) &= ~EPCTL_USBAEP;
        OTG_DIEPCTL(EP_NOTIF) &= ~EPCTL_USBAEP;
        OTG_DOEPCTL(EP_DATA) &= ~EPCTL_USBAEP;
        usb.configured = false;
        return;
    }
    flush_tx_fifo(EP_DATA);
    flush_tx_fifo(EP_NOTIF);
    OTG_DIEPCTL(EP_DATA) = DATA_MPS | EPCTL_USBAEP | (2u << EPCTL_EPTYP_POS)
                         | (EP_DATA << EPCTL_TXFNUM_POS) | EPCTL_SD0PID | EPCTL_SNAK;
    OTG_DIEPCTL(EP_NOTIF) = NOTIF_MPS | EPCTL_USBAEP | (3u << EPCTL_EPTYP_POS)
                          | (EP_NOTIF << EPCTL_TXFNUM_POS) | EPCTL_SD0PID | EPCTL_SNAK;
    OTG_DOEPCTL(EP_DATA) = DATA_MPS | EPCTL_USBAEP | (2u << EPCTL_EPTYP_POS) | EPCTL_SD0PID;
    OTG_DAINTMSK |= (1u << (16 + EP_DATA));
    ep1_arm_out();
    usb.configured = true;
}

/**************************** control requests ****************************/
static void handle_get_descriptor(const usb_setup_t *req)
{
    uint8_t type = (uint8_t) (req->wValue >> 8);
    uint8_t index = (uint8_t) (req->wValue & 0xFF);
    switch (type) {
        case DESC_DEVICE:
            ep0_send(device_desc, sizeof device_desc, req->wLength);
            return;
        case DESC_CONFIGURATION:
            ep0_send(config_desc, sizeof config_desc, req->wLength);
            return;
        case DESC_STRING: {
            const char *s = NULL;
            if (index == 0) { ep0_send(lang_desc, sizeof lang_desc, req->wLength); return; }
            if (index == 1) s = ident.manufacturer;
            if (index == 2) s = ident.product;
            if (index == 3) s = ident.serial;
            if (s) { ep0_send(usb.desc_buf, string_desc(s), req->wLength); return; }
            break;
        }
        default:
            break;   /* device qualifier etc.: full-speed only -> STALL */
    }
    ep0_stall();
}

static void handle_setup(void)
{
    const usb_setup_t *req = &usb.setup.req;
    static const uint8_t zero2[2] = {0, 0};
    uint8_t type = req->bmRequestType & 0x60u;   /* 0 standard, 0x20 class */

    usb.ep0_out_pending = false;

    if (type == 0x00u) {
        switch (req->bRequest) {
            case REQ_GET_DESCRIPTOR:
                handle_get_descriptor(req);
                return;
            case REQ_SET_ADDRESS:
                /* DWC2: latch the address now, the core answers the status
                 * stage still on address 0. */
                OTG_DCFG = (OTG_DCFG & ~DCFG_DAD_MSK)
                         | (((uint32_t) req->wValue & 0x7Fu) << DCFG_DAD_POS);
                ep0_status_in();
                return;
            case REQ_SET_CONFIGURATION:
                if ((req->wValue & 0xFF) > 1u) break;
                configure_endpoints((uint8_t) req->wValue);
                ep0_status_in();
                return;
            case REQ_GET_CONFIGURATION:
                ep0_send(&usb.config_value, 1, req->wLength);
                return;
            case REQ_GET_STATUS:
                ep0_send(zero2, 2, req->wLength);
                return;
            case REQ_GET_INTERFACE: {
                static const uint8_t alt0 = 0;
                ep0_send(&alt0, 1, req->wLength);
                return;
            }
            case REQ_CLEAR_FEATURE:
            case REQ_SET_FEATURE:
            case REQ_SET_INTERFACE:
                ep0_status_in();
                return;
            default:
                break;
        }
    } else if (type == 0x20u) {
        switch (req->bRequest) {
            case CDC_SET_LINE_CODING:
                usb.ep0_out_pending = true;     /* 7 bytes follow on EP0 OUT */
                usb.ep0_out_len = 0;
                return;
            case CDC_GET_LINE_CODING:
                ep0_send(usb.line_coding, sizeof usb.line_coding, req->wLength);
                return;
            case CDC_SET_CONTROL_LINE_STATE:
                set_line_state((req->wValue & 1u) != 0, (req->wValue & 2u) != 0);
                ep0_status_in();
                return;
            case CDC_SEND_BREAK:
                ep0_status_in();
                return;
            default:
                break;
        }
    }
    ep0_stall();
}

static void handle_ep0_out_done(void)
{
    if (usb.ep0_out_pending) {
        usb.ep0_out_pending = false;
        if (usb.setup.req.bRequest == CDC_SET_LINE_CODING) {
            uint32_t n = usb.ep0_out_len < sizeof usb.line_coding ? usb.ep0_out_len
                                                                 : sizeof usb.line_coding;
            for (uint32_t i = 0; i < n; i++) usb.line_coding[i] = usb.ep0_out[i];
        }
        ep0_status_in();
    }
    /* else: the zero-length OUT status stage of an IN request */
}

/**************************** bus events ****************************/
static void handle_usb_reset(void)
{
    flush_tx_fifo(0x10);
    flush_rx_fifo();
    for (uint32_t ep = 0; ep < NUM_EPS; ep++) {
        OTG_DIEPINT(ep) = 0xFFFFu;
        OTG_DOEPINT(ep) = 0xFFFFu;
        OTG_DIEPCTL(ep) &= ~EPCTL_STALL;
        OTG_DOEPCTL(ep) = (OTG_DOEPCTL(ep) & ~EPCTL_STALL) | EPCTL_SNAK;
    }
    OTG_DAINTMSK = (1u << 16);                  /* OUT EP0 */
    OTG_DOEPMSK = DOEPINT_STUP | EPINT_XFRC;
    OTG_DIEPMSK = 0;
    OTG_DCFG &= ~DCFG_DAD_MSK;
    usb.configured = false;
    usb.config_value = 0;
    usb.ep0_out_pending = false;
    if (usb.dtr) set_line_state(false, false);
    ep0_arm_out();
}

static void handle_rx_fifo(void)
{
    uint32_t sts = OTG_GRXSTSP;
    uint8_t ep = GRXSTS_EPNUM(sts);
    uint32_t bcnt = GRXSTS_BCNT(sts);

    switch (GRXSTS_PKTSTS(sts)) {
        case PKTSTS_SETUP_DATA:
            /* 8 bytes; with back-to-back SETUPs the last one wins */
            usb.setup.words[0] = OTG_FIFO(0);
            usb.setup.words[1] = OTG_FIFO(0);
            if (bcnt > 8u) fifo_read(NULL, bcnt - 8u);
            break;
        case PKTSTS_OUT_DATA:
            if (ep == 0) {
                uint32_t room = sizeof usb.ep0_out - usb.ep0_out_len;
                uint32_t take = bcnt < room ? bcnt : room;
                fifo_read(&usb.ep0_out[usb.ep0_out_len], take);
                if (bcnt > take) fifo_read(NULL, bcnt - take);
                usb.ep0_out_len += take;
            } else if (ep == EP_DATA) {
                fifo_read(usb.rx_packet, bcnt);
                if (bcnt && usb.rx_fn) usb.rx_fn(usb.rx_packet, bcnt, usb.rx_aux);
            } else {
                fifo_read(NULL, bcnt);
            }
            break;
        default:   /* OUT transfer / SETUP stage complete: no data in the FIFO */
            break;
    }
}

static void handle_out_endpoints(void)
{
    uint32_t daint = OTG_DAINT & OTG_DAINTMSK;

    if (daint & (1u << 16)) {
        uint32_t ints = OTG_DOEPINT(0);
        OTG_DOEPINT(0) = ints;
        if (ints & EPINT_XFRC) {
            handle_ep0_out_done();
        }
        if (ints & DOEPINT_STUP) {
            handle_setup();
        }
        ep0_arm_out();
    }
    if (daint & (1u << (16 + EP_DATA))) {
        uint32_t ints = OTG_DOEPINT(EP_DATA);
        OTG_DOEPINT(EP_DATA) = ints;
        if ((ints & EPINT_XFRC) && usb.configured) {
            ep1_arm_out();
        }
    }
}

void OTG_FS_IRQHandler(void)
{
    uint32_t sts = OTG_GINTSTS & OTG_GINTMSK;

    if (sts & GINT_USBRST) {
        OTG_GINTSTS = GINT_USBRST;
        handle_usb_reset();
    }
    if (sts & GINT_ENUMDNE) {
        OTG_GINTSTS = GINT_ENUMDNE;
        OTG_DIEPCTL(0) &= ~0x3u;                 /* EP0 MPSIZ = 64 bytes */
        OTG_DCTL |= DCTL_CGINAK;
        OTG_GUSBCFG = (OTG_GUSBCFG & ~GUSBCFG_TRDT_MSK) | (6u << GUSBCFG_TRDT_POS);
    }
    while (OTG_GINTSTS & GINT_RXFLVL) {
        handle_rx_fifo();
    }
    if (sts & GINT_OEPINT) {
        handle_out_endpoints();
    }
    if (sts & GINT_USBSUSP) {
        OTG_GINTSTS = GINT_USBSUSP;
    }
}

/**************************** public API ****************************/
void usb_cdc_register_rx_handler(usb_cdc_rx_callback_t fn, void *aux)
{
    usb.rx_aux = aux;
    usb.rx_fn = fn;
}

void usb_cdc_register_line_state_handler(usb_cdc_line_state_callback_t fn, void *aux)
{
    usb.ls_aux = aux;
    usb.ls_fn = fn;
}

bool usb_cdc_is_configured(void) { return usb.configured; }
bool usb_cdc_dtr(void) { return usb.dtr; }

void usb_cdc_init(const usb_cdc_config_t *cfg)
{
    if (cfg) ident = *cfg;
    device_desc[8]  = (uint8_t) ident.vid;
    device_desc[9]  = (uint8_t) (ident.vid >> 8);
    device_desc[10] = (uint8_t) ident.pid;
    device_desc[11] = (uint8_t) (ident.pid >> 8);

    // PA11 = DM, PA12 = DP (AF10)
    peripheral_gpio_enable(PERIPHERAL_GPIOA);
    const gpio_id_t pins[] = {A11, A12};
    for (uint32_t i = 0; i < 2; i++) {
        gpio_set_alternate_function(pins[i], AF10);
        gpio_set_output_speed(pins[i], VERY_HIGH_S);
        gpio_set_mode(pins[i], MODER_AF);
    }
    peripheral_usb_otg_fs_enable();

    // core: embedded FS PHY, soft reset, transceiver on
    OTG_GAHBCFG &= ~GAHBCFG_GINTMSK;
    OTG_GUSBCFG |= GUSBCFG_PHYSEL;
    while (!(OTG_GRSTCTL & GRSTCTL_AHBIDL));
    OTG_GRSTCTL |= GRSTCTL_CSRST;
    while (OTG_GRSTCTL & GRSTCTL_CSRST);
    for (volatile uint32_t i = 0; i < 1000u; i++);
    OTG_GCCFG |= GCCFG_PWRDWN;

    // force device mode (takes effect after >= 25 ms)
    OTG_GUSBCFG = (OTG_GUSBCFG & ~(GUSBCFG_FHMOD | GUSBCFG_FDMOD | GUSBCFG_TRDT_MSK))
                | GUSBCFG_FDMOD | (6u << GUSBCFG_TRDT_POS);
    for (volatile uint32_t i = 0; i < 2000000u; i++);   /* ~50 ms at 200 MHz */

    // device: no VBUS sensing on PA9, force B-session valid instead
    OTG_DCTL |= DCTL_SDIS;
    OTG_GCCFG &= ~GCCFG_VBDEN;
    OTG_GOTGCTL |= GOTGCTL_BVALOEN | GOTGCTL_BVALOVAL;
    OTG_PCGCCTL = 0;
    OTG_DCFG = (OTG_DCFG & ~0x3u) | DCFG_DSPD_FS;

    // FIFO RAM partition
    OTG_GRXFSIZ = RX_FIFO_WORDS;
    OTG_DIEPTXF0 = (TX0_FIFO_WORDS << 16) | RX_FIFO_WORDS;
    OTG_DIEPTXF(1) = (TX1_FIFO_WORDS << 16) | (RX_FIFO_WORDS + TX0_FIFO_WORDS);
    OTG_DIEPTXF(2) = (TX2_FIFO_WORDS << 16) | (RX_FIFO_WORDS + TX0_FIFO_WORDS + TX1_FIFO_WORDS);
    for (uint32_t n = 3; n < NUM_EPS; n++) OTG_DIEPTXF(n) = 0;
    flush_tx_fifo(0x10);
    flush_rx_fifo();

    // all endpoints idle, all interrupts cleared
    OTG_DIEPMSK = 0;
    OTG_DOEPMSK = 0;
    OTG_DAINTMSK = 0;
    for (uint32_t ep = 0; ep < NUM_EPS; ep++) {
        OTG_DIEPCTL(ep) = (OTG_DIEPCTL(ep) & EPCTL_EPENA) ? (EPCTL_EPDIS | EPCTL_SNAK) : 0;
        OTG_DIEPTSIZ(ep) = 0;
        OTG_DIEPINT(ep) = 0xFB7Fu;
        OTG_DOEPCTL(ep) = (OTG_DOEPCTL(ep) & EPCTL_EPENA) ? (EPCTL_EPDIS | EPCTL_SNAK) : 0;
        OTG_DOEPTSIZ(ep) = 0;
        OTG_DOEPINT(ep) = 0xFB7Fu;
    }
    OTG_GINTMSK = 0;
    OTG_GINTSTS = 0xBFFFFFFFu;
    OTG_GINTMSK = GINT_RXFLVL | GINT_USBRST | GINT_ENUMDNE | GINT_OEPINT | GINT_USBSUSP;

    interrupts_enable_source(INTERRUPT_SOURCE_OTG_FS);
    OTG_GAHBCFG |= GAHBCFG_GINTMSK;

    OTG_DCTL &= ~DCTL_SDIS;   // connect: D+ pull-up on
}

/* Stop a blocked IN transfer: NAK, disable, drop what is left in the FIFO. */
static void ep1_in_abort(void)
{
    if (OTG_DIEPCTL(EP_DATA) & EPCTL_EPENA) {
        OTG_DIEPCTL(EP_DATA) |= EPCTL_SNAK;
        OTG_DIEPCTL(EP_DATA) |= EPCTL_EPDIS;
        for (volatile uint32_t g = 0; (OTG_DIEPCTL(EP_DATA) & EPCTL_EPENA) && g < 100000u; g++);
    }
    flush_tx_fifo(EP_DATA);
    OTG_DIEPINT(EP_DATA) = 0xFFFFu;
}

static bool write_aborted(uint32_t epoch)
{
    return !usb.configured || usb.dtr_drops != epoch;
}

/* One programmed transfer of up to IN_CHUNK bytes (0 = ZLP). */
static bool ep1_in_xfer(const uint8_t *p, uint32_t n, uint32_t epoch)
{
    while (OTG_DIEPCTL(EP_DATA) & EPCTL_EPENA) {
        if (write_aborted(epoch)) { ep1_in_abort(); return false; }
    }
    uint32_t pkts = (n == 0) ? 1u : (n + DATA_MPS - 1u) / DATA_MPS;
    OTG_DIEPINT(EP_DATA) = EPINT_XFRC;
    OTG_DIEPTSIZ(EP_DATA) = (pkts << EPTSIZ_PKTCNT_POS) | n;
    OTG_DIEPCTL(EP_DATA) |= EPCTL_EPENA | EPCTL_CNAK;

    while (n > 0) {
        uint32_t plen = n < DATA_MPS ? n : DATA_MPS;
        uint32_t words = (plen + 3u) / 4u;
        while ((OTG_DTXFSTS(EP_DATA) & 0xFFFFu) < words) {
            if (write_aborted(epoch)) { ep1_in_abort(); return false; }
        }
        fifo_write(EP_DATA, p, plen);
        p += plen;
        n -= plen;
    }
    while (!(OTG_DIEPINT(EP_DATA) & EPINT_XFRC)) {
        if (write_aborted(epoch)) { ep1_in_abort(); return false; }
    }
    OTG_DIEPINT(EP_DATA) = EPINT_XFRC;
    return true;
}

int32_t usb_cdc_write(const void *buf, uint32_t len)
{
    const uint8_t *p = (const uint8_t *) buf;
    uint32_t epoch = usb.dtr_drops;
    uint32_t sent = 0;

    if (!usb.configured) return -1;
    while (sent < len) {
        uint32_t chunk = (len - sent) < IN_CHUNK ? (len - sent) : IN_CHUNK;
        if (!ep1_in_xfer(p + sent, chunk, epoch)) return -1;
        sent += chunk;
    }
    // a transfer ending on a full packet needs a ZLP to complete on the host
    if (len > 0 && (len % DATA_MPS) == 0) {
        if (!ep1_in_xfer(NULL, 0, epoch)) return -1;
    }
    return (int32_t) sent;
}
