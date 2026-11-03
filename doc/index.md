# stefi-lib

**A bare-metal C firmware framework for STM32G4/L4 (Cortex-M4) and STM32F7
(Cortex-M7) — no HAL, no CMSIS.** Every register access is hand-written and readable; the whole stack
from GPIO bit to OLED driver fits in your head. Built for and used in the
TH Nürnberg Embedded Lab.

[Start a project — stefi-quickstart →](https://github.com/kross11480/stefi-quickstart){ .md-button .md-button--primary }
[Read the getting-started guide](GETTING_STARTED.md){ .md-button }

## Hello world

This is [`examples/1-Blinky`](https://github.com/kross11480/stefi-lib/tree/main/examples/1-Blinky),
embedded straight from the repo — it always matches what actually compiles:

```c
--8<-- "examples/1-Blinky/blinky.c"
```

## Build and flash

```sh
cmake --preset debug
cmake --build --preset debug
openocd -f configs/st_nucleo_g4.cfg \
        -c "program cmake-build-debug/examples/1-Blinky/blinky verify reset exit"
```

`printf` output appears on the ST-Link virtual COM port (115200 baud) —
no debugger setup needed for logging.

## Why stefi-lib

<div class="grid cards" markdown>

- **No vendor HAL**

    Memory-mapped register structs, one source file per peripheral. You can
    read the entire path from your function call to the hardware bit.

- **Layered, strictly one-way**

    `app → components → board interface → HAL`. Swap the board at link time
    with `-DSTEFI_BOARD=…`; components never see a concrete board.

- **Two boards supported**

    STefi (NUCLEO-L476RG: LEDs, buttons, OLED, FRAM) and RobOhmobil
    (NUCLEO-G431KB: motors, line sensors, IMU).

- **Learn by example**

    [Nine examples](examples.md) walk up the stack, from raw-register blinky
    to a complete reaction game with RNG, buttons, OLED and UART.

</div>
