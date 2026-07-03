# Getting started

## Supported hardware

| Board (`STEFI_BOARD`) | MCU | Description |
|---|---|---|
| `nucleo_l476` (default) | STM32L476RG | STefi adapter board on NUCLEO-L476RG (LEDs, buttons, OLED, FRAM) |
| `rohmi_g431` | STM32G431KB | RobOhmobil robot on NUCLEO-G431KB (motors, line sensors, IMU) |

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

`printf` output goes to the ST-Link virtual COM port (USART2, 115200 baud).

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

## Things to know

- `board_init()` does **not** switch the clock to 80 MHz. Call
  `system_init()` yourself if you need it; `printf`/UART works either way
  (it runs from HSI16).
- Tests are on-hardware only (`tests/`), printing PASS/FAIL over UART; the
  GPIO loopback tests need jumper wires.
- Blocking by default: I2C, SPI and ADC drivers poll.