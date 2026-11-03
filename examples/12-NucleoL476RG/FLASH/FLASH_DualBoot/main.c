/* FLASH_DualBoot (STM32CubeL4, NUCLEO-L476RG) on stefi-lib.
 * Built twice: l4_flash_dualboot_bank1 (FLASH_BANK1) and _bank2.
 *
 *   1. flash the bank-2 image at 0x08080000:
 *        openocd -f configs/st_nucleo_l4.cfg \
 *          -c "program l4_flash_dualboot_bank2.bin 0x08080000 verify reset exit"
 *      (arm-none-eabi-objcopy -O binary l4_flash_dualboot_bank2 l4_flash_dualboot_bank2.bin)
 *   2. flash the bank-1 image normally.
 * Running from bank 1, LD2 toggles every 50 ms; from bank 2 LD2 stays on.
 * B1 toggles the BFB2 option (boot from bank 2) and resets: the MCU then
 * boots the other bank. Both images are linked at 0x08000000: with BFB2
 * set, bank 2 is mapped there. */
#include "l4_example.h"
#include "libstefi/flash.h"

_Noreturn static void error_flash(void)
{
    for (;;) { led_on(LED2); systick_delay_ms(100); led_off(LED2); systick_delay_ms(2000); }
}

void main(void)
{
    board_init();
    led_on(LED2);
    for (;;) {
        if (example_button_pressed()) {
            while (example_button_pressed());
            flash_set_dual_boot(!flash_dual_boot_enabled());   // resets on success
            error_flash();
        }
#if defined(FLASH_BANK1)
        led_toggle(LED2);
#endif
        systick_delay_ms(50);
    }
}
