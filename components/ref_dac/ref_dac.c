#include "ref_dac.h"

#include "stefi_board.h"

#define REF_DAC_BITS 12u
#define REF_DAC_MAX  ((1u << REF_DAC_BITS) - 1u)

static uint16_t current_code;

static void pin_out(gpio_id_t pin, sig_t level)
{
    peripheral_gpio_enable(gpio_get_port_from_portpin(pin));
    gpio_write(pin, level);
    gpio_set_output_type(pin, ref_dac.otype);
    gpio_set_mode(pin, MODER_OUTPUT);
}

void ref_dac_init(void)
{
    for (uint32_t i = 0; i < REF_DAC_BITS; i++) {
        pin_out(ref_dac.data[i], LOW);
    }
    pin_out(ref_dac.clear_n, HIGH);   // not clearing
    pin_out(ref_dac.load_n, HIGH);    // latch closed
    ref_dac_write(0);
}

void ref_dac_write(uint16_t code)
{
    if (code > REF_DAC_MAX) code = REF_DAC_MAX;

    gpio_write(ref_dac.load_n, HIGH);             // hold the latch
    for (uint32_t i = 0; i < REF_DAC_BITS; i++) {
        gpio_write(ref_dac.data[i], (code >> i) & 1u ? HIGH : LOW);
    }
    gpio_write(ref_dac.load_n, LOW);              // transparent: output updates
    current_code = code;
}

uint16_t ref_dac_get(void)
{
    return current_code;
}

void ref_dac_clear(void)
{
    gpio_write(ref_dac.clear_n, LOW);
    for (volatile uint32_t i = 0; i < 100u; i++);  // tCLR >= 100 ns
    gpio_write(ref_dac.clear_n, HIGH);
    current_code = 0;
}
