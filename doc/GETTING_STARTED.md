# Getting started

## Supported hardware

| Board (`STEFI_BOARD`) | MCU | Description |
|---|---|---|
| `nucleo_l476` (default) | STM32L476RG | STefi adapter board on NUCLEO-L476RG (LEDs, buttons, OLED, FRAM) |
| `rohmi_g431` | STM32G431KB | RobOhmobil robot on NUCLEO-G431KB (motors, line sensors, IMU) |
| `nucleo_f767zi` | STM32F767ZI | NUCLEO-F767ZI + USB-oscilloscope front-end (parallel ADC/DAC, trigger, USB CDC) |

## Prerequisites

- `arm-none-eabi-gcc` (with newlib-nano)
- CMake ≥ 3.24
- OpenOCD (for flashing via the on-board ST-Link)

## Build and flash

```sh
cmake --preset debug
cmake --build --preset debug
openocd -f configs/st_nucleo_g4.cfg \
        -c "program cmake-build-debug/examples/1-Blinky/blinky verify reset exit"
```

To build for the Rohmi instead: `cmake --preset debug -DSTEFI_BOARD=rohmi_g431`.

For the NUCLEO-F767ZI (Cortex-M7) use a separate build directory and the F7
OpenOCD config:

```sh
cmake -B cmake-build-f7 -DSTEFI_BOARD=nucleo_f767zi
cmake --build cmake-build-f7
openocd -f configs/st_nucleo_f7.cfg \
        -c "program cmake-build-f7/examples/10-UsbOscilloscope/usb_oscilloscope verify reset exit"
```

`printf` output goes to the ST-Link virtual COM port (USART2, 115200 baud;
USART3 on the NUCLEO-F767ZI).

## Architecture

Four one-way layers, `app → components → board interface → HAL`:

- **`src/`, `include/libstefi/`** — the HAL: one file per peripheral (GPIO,
  UART, timers, I2C, SPI, ADC, DMA, RNG, …). Link `stefi::hal`.
- **`boards/interface/`** — `stefi_board.h`, the contract any board
  implements (config tables, `board_init()`). Link `stefi::board_iface`.
- **`boards/<name>/`** — concrete boards; selected with `-DSTEFI_BOARD=…`.
  Link `stefi::board`.
- **`components/`** — board-agnostic drivers: SimpleIO (`stefi::simpleio`),
  OLED (`stefi::oled`), motor, line sensor, IMU.

`examples/` walks up the stack: 1-Blinky is raw HAL, 5–6 use components,
8/9 are whole-board smoke tests.

## MCU families

A board's optional `boards/<name>/board.cmake` selects the MCU family; the
build then compiles that family's HAL sources and defines `STEFI_MCU_STM32L4G4`
or `STEFI_MCU_STM32F7` for every target linking `stefi::hal`.

- **STM32L4G4** (L476, G431): the full HAL in `src/`.
- **STM32F7** (F767): GPIO/EXTI, timers, SysTick, interrupts and the ring
  buffer are shared with L4/G4 (only base addresses differ, chosen in
  `src/internal/`); RCC/clock, startup, USART3, DMA and USB live in
  `src/stm32f7/`. F7-only APIs: `libstefi/dma_stream.h` (stream DMA with
  double-buffer mode) and `libstefi/usb_cdc.h` (USB CDC virtual COM port on
  OTG_FS). I2C, SPI, ADC, RNG, WWDG and flash are not ported to the F7 yet.
  `board_init()` on the F767 *does* call `system_init()` (200 MHz): USB needs
  its 48 MHz clock.

## Things to know

- `board_init()` does **not** switch the clock to 80 MHz. Call
  `system_init()` yourself if you need it; `printf`/UART works either way
  (it runs from HSI16).
- Tests in `tests/L4` run on hardware, printing PASS/FAIL over UART; the
  GPIO loopback tests need jumper wires. `sh tests/host/run.sh` runs the
  host-side unit tests (portable components, oscilloscope protocol) with a
  native compiler.
- Blocking by default: I2C, SPI and ADC drivers poll.