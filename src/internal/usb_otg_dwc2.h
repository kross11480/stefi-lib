#pragma once
#include <stdint.h>

/**********************************************************************/
/* USB OTG_FS (Synopsys DWC2 core) — private register map             */
/* STM32F7: RM0410 §42, STM32L4: RM0351 §47 (same core, same base).   */
/* Device mode only; slave (non-DMA) FIFO access.                     */
/**********************************************************************/
#define OTG_FS_BASE 0x50000000u

#define OTG_REG(off) (*(volatile uint32_t *)(OTG_FS_BASE + (off)))

/* core global */
#define OTG_GOTGCTL     OTG_REG(0x000)
#define OTG_GAHBCFG     OTG_REG(0x008)
#define OTG_GUSBCFG     OTG_REG(0x00C)
#define OTG_GRSTCTL     OTG_REG(0x010)
#define OTG_GINTSTS     OTG_REG(0x014)
#define OTG_GINTMSK     OTG_REG(0x018)
#define OTG_GRXSTSP     OTG_REG(0x020)
#define OTG_GRXFSIZ     OTG_REG(0x024)
#define OTG_DIEPTXF0    OTG_REG(0x028)
#define OTG_GCCFG       OTG_REG(0x038)
#define OTG_DIEPTXF(n)  OTG_REG(0x104 + 4u * ((n) - 1u))   /* n = 1..5 */

/* device */
#define OTG_DCFG        OTG_REG(0x800)
#define OTG_DCTL        OTG_REG(0x804)
#define OTG_DSTS        OTG_REG(0x808)
#define OTG_DIEPMSK     OTG_REG(0x810)
#define OTG_DOEPMSK     OTG_REG(0x814)
#define OTG_DAINT       OTG_REG(0x818)
#define OTG_DAINTMSK    OTG_REG(0x81C)
#define OTG_DIEPCTL(n)  OTG_REG(0x900 + 0x20u * (n))
#define OTG_DIEPINT(n)  OTG_REG(0x908 + 0x20u * (n))
#define OTG_DIEPTSIZ(n) OTG_REG(0x910 + 0x20u * (n))
#define OTG_DTXFSTS(n)  OTG_REG(0x918 + 0x20u * (n))
#define OTG_DOEPCTL(n)  OTG_REG(0xB00 + 0x20u * (n))
#define OTG_DOEPINT(n)  OTG_REG(0xB08 + 0x20u * (n))
#define OTG_DOEPTSIZ(n) OTG_REG(0xB10 + 0x20u * (n))
#define OTG_PCGCCTL     OTG_REG(0xE00)
#define OTG_FIFO(n)     OTG_REG(0x1000 + 0x1000u * (n))

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
