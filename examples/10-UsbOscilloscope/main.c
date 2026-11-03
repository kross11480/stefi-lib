/*
 * USB oscilloscope on stefi-lib — NUCLEO-F767ZI or NUCLEO-L476RG + analog
 * front-end.
 *
 * Build: cmake -B build -DSTEFI_BOARD=nucleo_f767zi      (or nucleo_l476_scope)
 * Flash: openocd -f configs/st_nucleo_f7.cfg \
 *          -c "program build/examples/10-UsbOscilloscope/usb_oscilloscope verify reset exit"
 *        (L476: configs/st_nucleo_l4.cfg)
 *
 * Connect the USB device port (F767: CN13; L476: PA11/PA12 wired to a USB
 * connector) to the PC and start the PC software; debug output goes to the
 * ST-Link virtual COM port (F767: USART3, L476: USART2, 115200 baud).
 */
#include <stdio.h>

#include "board.h"
#include "scope.h"

#include "libstefi/interrupts.h"
#include "libstefi/usb_cdc.h"

void main(void)
{
    interrupts_global_disable();
    board_init();          // 200 MHz, USART3 printf, LEDs, button, SysTick
    scope_hw_init();       // ADC capture, reference DAC, trigger, button IRQ
    scope_fsm_init();
    scope_protocol_init(); // USB CDC device
    interrupts_global_enable();

    printf("usb-oscilloscope (stefi-lib) ready, waiting for the PC\r\n");

    for (;;) {
        // like V1: the state machine only runs while the PC holds the port
        // open (DTR); a reconnect starts over from the init state
        while (!usb_cdc_dtr()) { }
        scope_fsm_reset();
        while (usb_cdc_dtr()) {
            scope_protocol_poll();      // commands -> events
            scope_fsm_step();           // events -> state machine
        }
        scope_acquisition_stop();
    }
}
