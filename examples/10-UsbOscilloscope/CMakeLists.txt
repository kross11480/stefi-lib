# USB oscilloscope (kross11480/usb-oscilloscope) on stefi-lib.
# NUCLEO-F767ZI + analog front-end only: -DSTEFI_BOARD=nucleo_f767zi
if(NOT STEFI_BOARD STREQUAL "nucleo_f767zi")
    return()
endif()

add_executable(usb_oscilloscope
        main.c
        scope_hw.c
        scope_fsm.c
        scope_protocol.c
)

target_link_libraries(usb_oscilloscope PRIVATE
        stefi::board            # LEDs, button, clocks, pin tables
        stefi::simpleio         # led_*, button_*
        stefi::parallel_adc     # LTC1420 capture (TIM1 clock + DMA2 double buffer)
        stefi::ref_dac          # LTC1450 -> ADC reference voltage
        stefi::edge_trigger     # LT1714 comparator trigger
        stefi::cmd_line         # text command protocol
        stefi::fsm              # application state machine
        stefi::dsp              # moving-average filter
)
