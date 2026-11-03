/*
 * USB oscilloscope on stefi-lib — NUCLEO-F767ZI + analog front-end.
 *
 * Build: cmake --preset debug -DSTEFI_BOARD=nucleo_f767zi
 * Flash: openocd -f configs/st_nucleo_f7.cfg \
 *          -c "program cmake-build-debug/examples/10-UsbOscilloscope/usb_oscilloscope verify reset exit"
 *
 * Connect CN13 (USB OTG) to the PC and start the PC software; debug output
 * goes to the ST-Link virtual COM port (USART3, 115200 baud).
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
