#pragma once
#include <stdint.h>

/**********************************************************************/
/* USB OTG_FS (Synopsys DWC2 core) — private register map             */
/* STM32F7: RM0410 §42, STM32L4: RM0351 §47 (same core, same base).   */
/* Device mode only; slave (non-DMA) FIFO access.                     */
/**********************************************************************/
typedef struct {
    volatile uint32_t GOTGCTL;   /*!< OTG control and status,        Address offset: 0x000 */
    volatile uint32_t GOTGINT;   /*!< OTG interrupt,                 Address offset: 0x004 */
    volatile uint32_t GAHBCFG;   /*!< AHB configuration,             Address offset: 0x008 */
    volatile uint32_t GUSBCFG;   /*!< USB configuration,             Address offset: 0x00C */
    volatile uint32_t GRSTCTL;   /*!< reset,                         Address offset: 0x010 */
    volatile uint32_t GINTSTS;   /*!< core interrupt,                Address offset: 0x014 */
    volatile uint32_t GINTMSK;   /*!< interrupt mask,                Address offset: 0x018 */
    volatile uint32_t GRXSTSR;   /*!< receive status debug read,     Address offset: 0x01C */
    volatile uint32_t GRXSTSP;   /*!< receive status read and pop,   Address offset: 0x020 */
    volatile uint32_t GRXFSIZ;   /*!< receive FIFO size,             Address offset: 0x024 */
    volatile uint32_t DIEPTXF0;  /*!< EP0 transmit FIFO size,        Address offset: 0x028 */
    volatile uint32_t GNPTXSTS;  /*!< non-periodic TX FIFO status,   Address offset: 0x02C */
    uint32_t RESERVED0[2];       /*!< Reserved,                      Address offset: 0x030 */
    volatile uint32_t GCCFG;     /*!< general core configuration,    Address offset: 0x038 */
    volatile uint32_t CID;       /*!< core ID,                       Address offset: 0x03C */
    uint32_t RESERVED1[48];      /*!< Reserved,                      Address offset: 0x040 */
    volatile uint32_t HPTXFSIZ;  /*!< host periodic TX FIFO size,    Address offset: 0x100 */
    volatile uint32_t DIEPTXF[5];/*!< EP1..5 transmit FIFO size,     Address offset: 0x104 */
} OTG_Global_t;

typedef struct {
    volatile uint32_t DCFG;      /*!< device configuration,          Address offset: 0x800 */
    volatile uint32_t DCTL;      /*!< device control,                Address offset: 0x804 */
    volatile uint32_t DSTS;      /*!< device status,                 Address offset: 0x808 */
    uint32_t RESERVED0;          /*!< Reserved,                      Address offset: 0x80C */
    volatile uint32_t DIEPMSK;   /*!< IN endpoint interrupt mask,    Address offset: 0x810 */
    volatile uint32_t DOEPMSK;   /*!< OUT endpoint interrupt mask,   Address offset: 0x814 */
    volatile uint32_t DAINT;     /*!< all endpoints interrupt,       Address offset: 0x818 */
    volatile uint32_t DAINTMSK;  /*!< all endpoints interrupt mask,  Address offset: 0x81C */
} OTG_Device_t;

typedef struct {
    volatile uint32_t DIEPCTL;   /*!< IN endpoint control,           Address offset: 0x900 + 0x20 * n */
    uint32_t RESERVED0;
    volatile uint32_t DIEPINT;   /*!< IN endpoint interrupt,         Address offset: 0x908 + 0x20 * n */
    uint32_t RESERVED1;
    volatile uint32_t DIEPTSIZ;  /*!< IN endpoint transfer size,     Address offset: 0x910 + 0x20 * n */
    uint32_t RESERVED2;
    volatile uint32_t DTXFSTS;   /*!< IN endpoint TX FIFO status,    Address offset: 0x918 + 0x20 * n */
    uint32_t RESERVED3;
} OTG_INEP_t;

typedef struct {
    volatile uint32_t DOEPCTL;   /*!< OUT endpoint control,          Address offset: 0xB00 + 0x20 * n */
    uint32_t RESERVED0;
    volatile uint32_t DOEPINT;   /*!< OUT endpoint interrupt,        Address offset: 0xB08 + 0x20 * n */
    uint32_t RESERVED1;
    volatile uint32_t DOEPTSIZ;  /*!< OUT endpoint transfer size,    Address offset: 0xB10 + 0x20 * n */
    uint32_t RESERVED2[3];
} OTG_OUTEP_t;

#define OTG_FS_BASE  0x50000000
#define OTG          ((OTG_Global_t *) OTG_FS_BASE)
#define OTG_DEVICE   ((OTG_Device_t *) (OTG_FS_BASE + 0x800))
#define OTG_INEP(n)  ((OTG_INEP_t *) (OTG_FS_BASE + 0x900 + 0x20 * (n)))
#define OTG_OUTEP(n) ((OTG_OUTEP_t *) (OTG_FS_BASE + 0xB00 + 0x20 * (n)))
#define OTG_PCGCCTL  (*(volatile uint32_t *) (OTG_FS_BASE + 0xE00))   /* power and clock gating */
#define OTG_FIFO(n)  (*(volatile uint32_t *) (OTG_FS_BASE + 0x1000 + 0x1000 * (n)))   /* push/pop */

/* GAHBCFG */
#define GAHBCFG_GINTMSK   (1u << 0)
/* GUSBCFG */
#define GUSBCFG_PHYSEL    (1u << 6)
#define GUSBCFG_TRDT_POS  10u
#define GUSBCFG_TRDT_MSK  (0xFu << 10)
#define GUSBCFG_FHMOD     (1u << 29)
#define GUSBCFG_FDMOD     (1u << 30)
/* GRSTCTL */
#define GRSTCTL_CSRST     (1u << 0)
#define GRSTCTL_RXFFLSH   (1u << 4)
#define GRSTCTL_TXFFLSH   (1u << 5)
#define GRSTCTL_TXFNUM_ALL (0x10u << 6)
#define GRSTCTL_AHBIDL    (1u << 31)
/* GINTSTS / GINTMSK */
#define GINT_RXFLVL       (1u << 4)
#define GINT_USBSUSP      (1u << 11)
#define GINT_USBRST       (1u << 12)
#define GINT_ENUMDNE      (1u << 13)
#define GINT_IEPINT       (1u << 18)
#define GINT_OEPINT       (1u << 19)
#define GINT_WKUPINT      (1u << 31)
/* GCCFG (F7 / L4 core) */
#define GCCFG_PWRDWN      (1u << 16)
#define GCCFG_VBDEN       (1u << 21)
/* GOTGCTL */
#define GOTGCTL_BVALOEN   (1u << 6)
#define GOTGCTL_BVALOVAL  (1u << 7)
/* DCFG */
#define DCFG_DSPD_FS      (3u << 0)    /* full speed, internal PHY */
#define DCFG_DAD_POS      4u
#define DCFG_DAD_MSK      (0x7Fu << 4)
/* DCTL */
#define DCTL_SDIS         (1u << 1)
#define DCTL_CGINAK       (1u << 8)
/* D{I,O}EPCTL */
#define EPCTL_USBAEP      (1u << 15)
#define EPCTL_EPTYP_POS   18u
#define EPCTL_STALL       (1u << 21)
#define EPCTL_TXFNUM_POS  22u
#define EPCTL_CNAK        (1u << 26)
#define EPCTL_SNAK        (1u << 27)
#define EPCTL_SD0PID      (1u << 28)
#define EPCTL_EPDIS       (1u << 30)
#define EPCTL_EPENA       (1u << 31)
/* D{I,O}EPINT */
#define EPINT_XFRC        (1u << 0)
#define DOEPINT_STUP      (1u << 3)
/* D{I,O}EPTSIZ */
#define EPTSIZ_PKTCNT_POS 19u
#define DOEPTSIZ0_STUPCNT_3 (3u << 29)
/* GRXSTSP */
#define GRXSTS_EPNUM(x)   ((x) & 0xFu)
#define GRXSTS_BCNT(x)    (((x) >> 4) & 0x7FFu)
#define GRXSTS_PKTSTS(x)  (((x) >> 17) & 0xFu)
#define PKTSTS_OUT_DATA   2u
#define PKTSTS_OUT_DONE   3u
#define PKTSTS_SETUP_DONE 4u
#define PKTSTS_SETUP_DATA 6u
