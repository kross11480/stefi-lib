# stefi-lib

A bare-metal C firmware framework for STM32G4/L4 (Cortex-M4), used in the
TH Nürnberg Embedded Lab.

Supported boards, prerequisites, and build/flash instructions:
[doc/GETTING_STARTED.md](doc/GETTING_STARTED.md).

## Hello world

Raw HAL — no board layer, just registers behind a thin API:

```c
#include <libstefi/gpio.h>
#include <libstefi/peripheral.h>
#include <libstefi/util.h>

void main(void) {
    peripheral_gpio_enable(PERIPHERAL_GPIOC);   // clock gating first, always
    gpio_set_mode(C5, MODER_OUTPUT);            // LED1 on the STefi
    while (1) {
        gpio_toggle(C5);
        soft_delay_ms(50);
    }
}
```

Or at the board level — named devices, debounced buttons, SysTick:

```c
#include "board.h"
#include "led.h"

void main(void) {
    board_init();               // UART, LEDs, buttons, debounce timer, SysTick
    led_init(LED0_RED, OUTPUT);
    while (1) {
        led_toggle(LED0_RED);
        systick_delay_ms(500);
    }
}
```
## Using stefi-lib in your own project

The easiest way to start is the
[stefi-quickstart](https://github.com/kross11480/stefi-quickstart) template:
a ready-made application repo (CMake presets, pinned stefi-lib, `flash`
target) — clone it, build, and you have a running blinky to edit.

## License

[MIT](LICENSE)
