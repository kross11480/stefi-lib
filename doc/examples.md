# Examples

The [`examples/`](https://github.com/kross11480/stefi-lib/tree/main/examples)
directory walks up the stack layer by layer: example 1 touches only the raw
HAL, 5–6 use the component layer, 8–9 smoke-test a whole board. Build any of
them with the normal presets; the ELF ends up in
`cmake-build-debug/examples/<name>/`.

<div class="grid cards" markdown>

- **1 — Blinky**

    Raw HAL, no board layer: enable the GPIO clock, set a pin to output,
    toggle in a loop. The canonical first flash.

- **2 — Elegoo Sensor Kit**

    One small driver per kit module: buzzer, ultrasound, encoder, RGB LED,
    touch, obstacle avoidance, microphone, motion (I2C and SPI), and more.

- **3 — BitBang**

    Protocols driven by hand from GPIO timing — includes a DHT11
    temperature/humidity sensor read.

- **4 — TestShell**

    A small interactive command shell over the UART virtual COM port.

- **5 — TestModules**

    The component layer on the STefi: LED chaser, debounced buttons, button
    interrupts, periodic timers, PWM fade, input capture, OLED output.

- **6 — TestApp**

    A complete application: a reaction game using hardware RNG, buttons,
    the SSD1306 OLED and UART.

- **7 — TestWWDG**

    Window watchdog configuration and reset behaviour.

- **8 — Rohmi BoardTest**

    Whole-board smoke test for the RobOhmobil (motors, line sensors, IMU).
    Builds only with `-DSTEFI_BOARD=rohmi_g431`.

- **9 — Stefi BoardTest**

    Whole-board smoke test for the STefi adapter board
    (LEDs, buttons, OLED, FRAM). Builds with the default board.

- **10 — USB Oscilloscope**

    The [USB oscilloscope](usb-oscilloscope.md) as a stefi-lib application:
    10 MSPS parallel-ADC capture by DMA, comparator trigger, DAC-set reference
    voltage and a USB CDC link to the PC software.
    Builds with `-DSTEFI_BOARD=nucleo_f767zi` or `-DSTEFI_BOARD=nucleo_l476_scope`.

- **11 — NUCLEO-G431KB (STM32CubeG4 examples)**

    All 21 of ST's NUCLEO-G431KB examples on stefi-lib: CORDIC, FMAC,
    flash, GPIO, IWDG, WWDG, low-power modes with RTC wakeup, SPI, timers
    and UART — see [the overview](g431-cube-examples.md).
    Builds with `-DSTEFI_BOARD=nucleo_g431kb`.

- **12 — NUCLEO-L476RG (STM32CubeL4 examples)**

    All 72 of ST's NUCLEO-L476RG examples on stefi-lib: COMP, CRC, Cortex
    MPU/privilege, DAC, firewall, dual-bank flash, GPIO, I2C, IWDG/WWDG, LPTIM,
    OPAMP, every low-power mode, RCC, RTC, SPI, timers and UART — see
    [the overview](l476-cube-examples.md).
    Builds with `-DSTEFI_BOARD=nucleo_l476rg`.

</div>
