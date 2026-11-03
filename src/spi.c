#include "libstefi/spi.h"

#include <assert.h>
#include <stddef.h>
#include <libstefi/peripheral.h>
#include <libstefi/util.h>

#include "internal/spi_internal.h"
#include "internal/mcu_id.h"
#include "libstefi/dma_channel.h"

SPI_t *spi_peripherals[] = {(SPI_t*) SPI1_BASE, NULL, NULL};

void spi_init(const peripheral_spi_t spi_id) {
    assert(spi_id >= 0);
    peripheral_spi_enable(spi_id);

    SPI_t *spi = spi_peripherals[spi_id];
    spi->CR1 |= BIT(4); //bitrate = clock/8
    spi->CR1 |= BIT(2); //Master Mode
    spi->CR1 |= BIT(8) | BIT(9);// Software NSS, set SSI

    spi->CR2 |= BIT(12);
    spi->CR1 |= BIT(6);   // SPI enable
}

void spi_gpio_init(gpio_id_t cs_pin, gpio_id_t sck_pin, gpio_id_t miso_pin, gpio_id_t mosi_pin) {
    peripheral_gpio_enable(gpio_get_port_from_portpin(cs_pin));
    peripheral_gpio_enable(gpio_get_port_from_portpin(sck_pin));
    peripheral_gpio_enable(gpio_get_port_from_portpin(miso_pin));
    peripheral_gpio_enable(gpio_get_port_from_portpin(mosi_pin));

    gpio_set_alternate_function(cs_pin, AF5);//all spi on AF5
    gpio_set_alternate_function(sck_pin, AF5);
    gpio_set_alternate_function(miso_pin, AF5);//all spi on AF5
    gpio_set_alternate_function(mosi_pin, AF5);

    gpio_set_mode(cs_pin, MODER_AF);
    gpio_set_mode(sck_pin, MODER_AF);
    gpio_set_mode(miso_pin, MODER_AF);
    gpio_set_mode(mosi_pin, MODER_AF);

    gpio_set_output_speed(sck_pin, HIGH_S);
    gpio_set_output_speed(miso_pin, HIGH_S);
    gpio_set_output_speed(mosi_pin, HIGH_S);

    gpio_set_mode(A4, MODER_OUTPUT);
}

uint8_t spi_transfer(const peripheral_spi_t spi_id, uint8_t data) {
    assert(spi_id >= 0);
    SPI_t *spi = spi_peripherals[spi_id];

    uint8_t rx_data;
    // Wait until TX buffer is empty
    while (!( spi->SR & BIT(1) ));
    *(uint8_t *)&(spi->DR) = data;

    // Wait until RX buffer is not empty
    while (!(spi->SR & BIT(0)));
    rx_data = (uint8_t)spi->DR;
    return rx_data; // Return received data
}

/**************************** configurable SPI ****************************/
#define CR1_CPHA     BIT(0)
#define CR1_CPOL     BIT(1)
#define CR1_MSTR     BIT(2)
#define CR1_SPE      BIT(6)
#define CR1_LSBFIRST BIT(7)
#define CR1_SSI      BIT(8)
#define CR1_SSM      BIT(9)
#define CR2_RXDMAEN  BIT(0)
#define CR2_TXDMAEN  BIT(1)
#define CR2_FRXTH    BIT(12)
#define SR_RXNE      BIT(0)
#define SR_BSY       BIT(7)
#define SR_FTLVL     (3u << 11)
#define SR_ERR       (BIT(4) | BIT(5) | BIT(6) | BIT(8))   /* CRCERR MODF OVR FRE */

#define SPI_DMA_CH_RX 2u
#define SPI_DMA_CH_TX 3u

static struct {
    volatile bool busy;
    bool master;
    spi_callback_t done;
    void *aux;
} xfer;

void spi_configure(const peripheral_spi_t spi_id, const spi_config_t *cfg)
{
    assert(spi_id == PERIPHERAL_SPI1);
    peripheral_spi_enable(spi_id);
    SPI_t *spi = spi_peripherals[spi_id];

    spi->CR1 = 0;
    uint32_t cr1 = CR1_SSM;
    if (cfg->mode & 1u) cr1 |= CR1_CPHA;
    if (cfg->mode & 2u) cr1 |= CR1_CPOL;
    if (cfg->lsb_first) cr1 |= CR1_LSBFIRST;
    if (cfg->master) {
        uint8_t br = (cfg->baud_div_log2 < 1) ? 0 : (uint8_t) (cfg->baud_div_log2 - 1u);
        cr1 |= CR1_MSTR | CR1_SSI | ((uint32_t) (br & 7u) << 3);
    }                                  // slave: SSM = 1, SSI = 0 -> selected
    spi->CR2 = (0x7u << 8) | CR2_FRXTH;   // 8-bit frames, RXNE at 1 byte
    spi->CR1 = cr1;
    xfer.master = cfg->master;
}

void spi_gpio_init_pins(gpio_id_t sck_pin, gpio_id_t miso_pin, gpio_id_t mosi_pin)
{
    const gpio_id_t pins[3] = {sck_pin, miso_pin, mosi_pin};
    for (uint32_t i = 0; i < 3; i++) {
        peripheral_gpio_enable(gpio_get_port_from_portpin(pins[i]));
        gpio_set_alternate_function(pins[i], AF5);
        gpio_set_output_speed(pins[i], HIGH_S);
        gpio_set_mode(pins[i], MODER_AF);
    }
}

static void spi_dma_route(bool tx, uint8_t *req)
{
    // G4: DMAMUX SPI1_RX = 10, SPI1_TX = 11; L4: CSELR C2S/C3S = 1
    if (mcu_is_g4()) *req = tx ? 11u : 10u; else *req = 1u;
}

static void on_rx_done(void *aux, uint32_t flags)
{
    (void) aux;
    if (!(flags & (DMA_CHANNEL_FLAG_TC | DMA_CHANNEL_FLAG_TE))) return;
    SPI_t *spi = spi_peripherals[PERIPHERAL_SPI1];
    bool error = (flags & DMA_CHANNEL_FLAG_TE) || (spi->SR & SR_ERR);
    if (xfer.master) {
        while (spi->SR & SR_FTLVL);
        while (spi->SR & SR_BSY);
    }
    spi->CR2 &= ~(CR2_TXDMAEN | CR2_RXDMAEN);
    spi->CR1 &= ~CR1_SPE;
    dma_channel_disable(PERIPHERAL_DMA1, SPI_DMA_CH_TX);
    dma_channel_disable(PERIPHERAL_DMA1, SPI_DMA_CH_RX);
    xfer.busy = false;
    if (xfer.done) xfer.done(xfer.aux, error);
}

void spi_transfer_dma(const peripheral_spi_t spi_id, const void *tx, void *rx, uint16_t len,
                      spi_callback_t done, void *aux)
{
    SPI_t *spi = spi_peripherals[spi_id];
    xfer.done = done;
    xfer.aux = aux;
    xfer.busy = true;

    uint8_t req_rx, req_tx;
    spi_dma_route(false, &req_rx);
    spi_dma_route(true, &req_tx);
    const dma_channel_config_t rx_cfg = {
        .dma = PERIPHERAL_DMA1, .channel = SPI_DMA_CH_RX, .request = req_rx,
        .dir = DMA_DIR_PERIPH_TO_MEM, .mem_inc = true, .priority = DMA_PRIORITY_HIGH,
        .irq_tc = true, .irq_te = true,
    };
    const dma_channel_config_t tx_cfg = {
        .dma = PERIPHERAL_DMA1, .channel = SPI_DMA_CH_TX, .request = req_tx,
        .dir = DMA_DIR_MEM_TO_PERIPH, .mem_inc = true, .priority = DMA_PRIORITY_LOW,
    };

    // RM0440 §39.4.9: RXDMAEN, channels, TXDMAEN, then SPE
    spi->CR1 &= ~CR1_SPE;
    while (spi->SR & SR_RXNE) (void) *(volatile uint8_t *) &spi->DR;   // drain
    spi->CR2 |= CR2_RXDMAEN;
    dma_channel_register_handler(PERIPHERAL_DMA1, SPI_DMA_CH_RX, on_rx_done, NULL);
    dma_channel_configure(&rx_cfg, (volatile void *) &spi->DR, rx, len);
    dma_channel_configure(&tx_cfg, (volatile void *) &spi->DR, (void *) tx, len);
    dma_channel_enable(PERIPHERAL_DMA1, SPI_DMA_CH_RX);
    dma_channel_enable(PERIPHERAL_DMA1, SPI_DMA_CH_TX);
    spi->CR2 |= CR2_TXDMAEN;
    spi->CR1 |= CR1_SPE;
}

bool spi_busy(const peripheral_spi_t spi_id)
{
    (void) spi_id;
    return xfer.busy;
}
