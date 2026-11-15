# USB oscilloscope

The [usb-oscilloscope](https://github.com/kross11480/usb-oscilloscope)
project (efi faculty, TH Nürnberg, summer semester 2025) built a 10 MSPS
single-channel USB oscilloscope: an analog front-end board on a
NUCLEO-F767ZI, streaming acquisitions to a Python GUI. Its firmware
(`USB_Oszi_Firmware_V1`) was written against the ST HAL, CubeMX code and the
ST USB device middleware.

[`examples/10-UsbOscilloscope`](https://github.com/kross11480/stefi-lib/tree/main/examples/10-UsbOscilloscope)
is the same firmware as a stefi-lib application: **no vendor code**, the
hardware behind board tables and reusable components, and the **wire
protocol unchanged** — the original PC software
(`src/python/software/software_v1`) talks to it as-is.

## Build and flash

```sh
cmake -B cmake-build-f7 -DSTEFI_BOARD=nucleo_f767zi
cmake --build cmake-build-f7
openocd -f configs/st_nucleo_f7.cfg \
        -c "program cmake-build-f7/examples/10-UsbOscilloscope/usb_oscilloscope verify reset exit"
```

Plug the user USB connector (CN13) into the PC; the board enumerates as a
CDC virtual COM port (VID:PID `0483:5740`, as with the ST stack). Debug
`printf` goes to the ST-Link COM port (USART3, 115200 baud).

## Port to the NUCLEO-L476RG

The same application also builds for an L476 (`-DSTEFI_BOARD=nucleo_l476_scope`,
flash with `configs/st_nucleo_l4.cfg`). The front-end PCB is made for the
NUCLEO-144 headers, so on the NUCLEO-64 it is wired by hand:

| Signal | F767 | L476 |
|---|---|---|
| ADC D0..D11, OF | PF3..PF14, PF15 | PC0..PC11, PC12 |
| ADC clock (TIM1_CH1) | PE9 | PA8 |
| Trigger (comparator) | PF2 / EXTI2 | PA0 / EXTI0 |
| DAC D0..D11, /CLR, /LD | PE2..PE8, PE10..PE14, PE0, PE1 | PB0..PB11, PB12, PB13 |
| USB D-/D+ | CN13 | PA11/PA12 to a USB connector |
| Capture DMA | DMA2 stream 5 ch 6 (double buffer) | DMA1 channel 6, CSELR 7 (single transfer) |
| printf | USART3 | USART2 |
| Status LEDs | LD1..LD3 (state 1..6) | LD2 only (bit 0) |

What changes with the smaller MCU:

- **Record length 32 768 samples** (65 536 bytes) instead of 100 000: the
  L476 has 128 KB RAM. The PC software has `datalength = 200_000` hard-coded
  in `usb_oszi_pc_fsm.py` — set it to `65_536`, or query the board with the
  new `GET_NUM_SAMPLES` command (`NUM_SAMPLES: 32768`).
- **Max. 5 MSPS** (board table; conservative, not yet measured): one DMA
  beat per sample at 80 MHz. Rates are 80 MHz / n.
- **Moving average in place** (no RAM for a second buffer), windows up to
  1025 points; wider requests report `MOVING_AVERAGE: 0`.
- **USB clock**: MSI at 48 MHz, hardware-trimmed by the 32.768 kHz LSE
  crystal of the NUCLEO-L476RG (`usb_cdc_init()` starts it; the first start
  waits for the LSE, up to about a second).
- **5 V pull-ups on the DAC lines** need 5 V tolerant pins — check
  DS10198 for PB0..PB13, or pull up to 3.3 V (enough for the LTC1450).

## Layers

| Layer | What the oscilloscope uses |
|---|---|
| App (`examples/10-UsbOscilloscope`) | `scope_fsm.c` acquisition state machine, `scope_protocol.c` PC commands, `scope_hw.c` actions |
| Components | `parallel_adc` (LTC1420 capture), `ref_dac` (LTC1450), `edge_trigger` (LT1714 comparator), `cmd_line`, `event_fsm`, `dsp`, SimpleIO (LEDs, button) |
| Board (`boards/nucleo_f767zi`) | pin/timer/DMA tables for the front-end, 200 MHz clock in `board_init()` |
| HAL | GPIO/EXTI, timers (+ DMA requests), DMA (`dma_stream.h` F7 / `dma_channel.h` L4/G4), USB CDC (`usb_cdc.h`, F7 + L4), UART, NVIC priorities |

What each piece replaces from V1:

| V1 (`Core/Src`, ST middleware) | stefi-lib |
|---|---|
| `SystemClock_Config()` (HAL RCC), PLLSAI 48 MHz | `system_init()` in `src/stm32f7/system.c` |
| `MX_USB_DEVICE_Init`, `usbd_*`, `usbd_cdc_if.c` | `usb_cdc_init()` / `usb_cdc_write()` (bare-metal OTG_FS CDC-ACM) |
| `TIM1_Init`, `DMA_Init`, `DMA2_Stream5_IRQHandler`, `determineBufferAddresses` | `parallel_adc` component on `timer_*` + `dma_stream_*` |
| `configureReferenceVoltage` (GPIOE bit packing) | `ref_dac_write()` |
| `EXTI_Init` / `EXTI2_IRQHandler` / `configureTrigger` | `edge_trigger` component |
| `GPIO_Init` (LEDs, B1), `TIM3` debounce | SimpleIO `led_*`, `button_interrupt_init()` |
| `fsm.c`, `queue.c` | `event_fsm` component |
| `receiveCommand`, `processCommand` | `cmd_line` component + command table |
| `preprocessData`, `applyMovingAverageFilter` | `parallel_adc_convert()`, `dsp_moving_average_i16()` |

## Signal path

```
LT1714 comparator ──► PF2 / EXTI2 ──► edge_trigger ISR ──► parallel_adc_start()
                                                              │ (enables DMA2 stream 5)
TIM1 CH1 PWM (PE9) ──► LTC1420 CLK                            ▼
TIM1 update ── DMA request ──► DMA2 S5 ch6: GPIOF->IDR ──► RAM, 2 x 50 000 samples
                                (double-buffer mode, ISR chains the segments)
```

`timer_clk / sample_rate` ticks per sample (TIM1 kernel 100 MHz): 20 kHz to
10 MHz, reported back as the rate actually set. The DMA reads the port on
the timer update event, half a clock period away from the ADC clock edge.
Each 16-bit port snapshot holds D0..D11 on PF3..PF14 and the overflow flag
on PF15; `parallel_adc_convert()` sign-extends the 2's complement data.

NVIC priorities: capture DMA 0, trigger 1, USB 6, button 8 — the USB stack
can never delay the start or the chaining of a capture.

## Protocol

Newline-terminated text commands from the PC, replies from the scope:

| PC → scope | scope → PC |
|---|---|
| `RESET` | (back to the init state) |
| `APP` | `STATE:IDLE` |
| `CONFIGURE_FREQ <Hz>` | `SAMPLING_FREQUENCY: <Hz set>` |
| `CONFIGURE_REF_VOLT <mV>` | `REFERENCE_VOLTAGE: <mV>` (ADC reference = 4.096 V − DAC) |
| `CONFIGURE_TRIG_RISE_EDGE` / `..._FALL_EDGE` | `TRIGGER:RISING_EDGE` / `TRIGGER:FALLING_EDGE` |
| `CONFIGURE_MOV_AVG <points>` | `MOVING_AVERAGE: <points>` (odd; 0 or even = off) |
| `RUN_START` | `STATE:ACQUISITION_START`, after the trigger `STATE:ACQUISITION_DONE` |
| `READY_FOR_DATA` | 200 000 bytes (100 000 × int16, little endian) + 1 overflow byte (`0xFF`/`0x00`) |
| `RECEIVED` | `STATE:WAIT_FOR_ACK` |
| `CONTINUE` / `STOP` | next acquisition / `STATE:IDLE` |
| `GET_NUM_SAMPLES` (not in V1) | `NUM_SAMPLES: <n>` (100000 F767, 32768 L476) |
| anything else | `ERR:UNKNOWN_CMD` |

The user button B1 forces an acquisition while waiting for the trigger.
Status LEDs (LD1 green = bit 0, LD2 blue = bit 1, LD3 red = bit 2) show the
state number 1..6 as in V1.

## Differences from V1

- Commands are parsed in the main loop (V1: inside the USB interrupt), so
  a slow reply can never stall the USB stack.
- `RESET` also restores the default configuration (10 MHz, 4095 mV, rising
  edge, filter off); V1 reset the timer to 10 MHz but kept reporting the old
  rate.
- The moving-average output keeps the unfiltered samples at the two edges
  where the window does not fit (V1 sent zeros there).
- Reconnecting the PC (DTR drop and re-assert) restarts from the init state.

## Testing status

`sh tests/host/run.sh` replays the PC software's command sequence against
the real state machine and protocol code and checks the conversion and
filter against the V1 algorithms. The hardware drivers (USB CDC, capture
DMA, clocks) are build-tested; bring-up on the NUCLEO-F767ZI / NUCLEO-L476RG
with the front-end board is still to be done.
