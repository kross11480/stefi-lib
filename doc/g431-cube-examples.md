# STM32CubeG4 examples on stefi-lib

ST ships 21 register-level demos for the NUCLEO-G431KB in
[STM32CubeG4 / Projects/NUCLEO-G431KB/Examples](https://github.com/STMicroelectronics/STM32CubeG4/tree/master/Projects/NUCLEO-G431KB/Examples).
[`examples/11-NucleoG431KB`](https://github.com/kross11480/stefi-lib/tree/main/examples/11-NucleoG431KB)
re-implements every one of them on stefi-lib — same pins, timings and LED2
signalling, without the ST HAL.

```sh
cmake -B cmake-build-g4 -DSTEFI_BOARD=nucleo_g431kb
cmake --build cmake-build-g4
openocd -f configs/st_nucleo_g4.cfg \
        -c "program cmake-build-g4/examples/11-NucleoG431KB/g4_gpio_iotoggle verify reset exit"
```

Board `nucleo_g431kb` is the bare NUCLEO-32: LD2 on PB8, the "jumper"
input PA12 (Arduino D2) to GND as `BUTTON_D2`, ST-Link VCP on USART2, core
at 170 MHz (examples that ST runs at 150 MHz switch with
`system_init_g4(150000000)`).

LED2 conventions (from ST's examples): **on** = passed, **blinking 1 s** =
wrong result, **100 ms on / 2 s off** = a peripheral operation failed.

## Examples and the drivers they use

| ST example | stefi executable(s) | stefi APIs |
|---|---|---|
| CORDIC_Sin_DMA | `g4_cordic_sin_dma` | `cordic.h` (new), `dma_channel.h` |
| FLASH_EraseProgram | `g4_flash_erase_program` | `flash.h`: `flash_erase_pages`, `flash_write` |
| FLASH_FastProgram | `g4_flash_fast_program` | `flash.h`: `flash_fast_program_row` (RAM routine), `flash_mass_erase` |
| FLASH_WriteProtection | `g4_flash_wrp_enable`, `_disable`, `_program` | `flash.h`: `flash_get/set_write_protection` (option bytes) |
| FMAC_IIR_PollingToDMA | `g4_fmac_iir_polling_to_dma` | `fmac.h` (new): buffers, preload, IIR, DMA read, `fmac_model_iir` |
| GPIO_IOToggle | `g4_gpio_iotoggle` | `gpio.h` |
| IWDG_WindowMode | `g4_iwdg_window_mode` | `iwdg.h` (new), `system.h` reset flags, EXTI |
| PWR_STANDBY_RTC | `g4_pwr_standby_rtc` | `pwr.h`, `rtc.h` (new) |
| PWR_STOP0_RTC / PWR_STOP1_RTC | `g4_pwr_stop0_rtc`, `g4_pwr_stop1_rtc` | `pwr.h`, `rtc.h`, `system_init_g4` after wakeup |
| SPI_FullDuplex_ComDMA_Master / _Slave | `g4_spi_fullduplex_dma_master`, `_slave` | `spi.h`: `spi_configure`, `spi_transfer_dma` |
| TIM_CascadeSynchro | `g4_tim_cascade_synchro` | `timer.h`: `timer_set_master_mode`, `timer_set_slave_mode` |
| TIM_DMA | `g4_tim_dma` | `timer.h`: `timer_ccr_address`, `timer_enable_dma_request`; `dma_channel.h` circular |
| TIM_InputCapture | `g4_tim_input_capture` | `timer.h` input capture, TIM1_CC interrupt |
| TIM_PWMInput | `g4_tim_pwm_input` | `timer.h`: `timer_set_ic_source` (indirect), slave reset mode |
| TIM_PWMOutput | `g4_tim_pwm_output` | `timer.h` PWM, `timer_set_oc_polarity` |
| UART_HyperTerminal_DMA | `g4_uart_hyperterminal_dma` | `usart.h` (new): 7O1, DMA TX/RX |
| UART_TwoBoards_ComIT | `g4_uart_twoboards_it_tx`, `_rx` | `usart.h` interrupt mode, USART1 |
| UART_TwoBoards_ComPolling | `g4_uart_twoboards_polling_tx`, `_rx` | `usart.h` polling with timeouts |
| WWDG_Example | `g4_wwdg_example` | `wwdg.h`: `wwdg_start`, `wwdg_refresh` |

ST's `TRANSMITTER_BOARD`, `WRITE_PROTECTION_*` and master/slave project
variants are separate executables here.

## New and extended drivers (STM32L4 / STM32G4)

| | |
|---|---|
| `system.h` | `system_init_g4(hz)`: PLL from HSI16 up to 170 MHz (boost mode, wait states), `system_get_core_clock()`, reset-cause flags, `system_reset()` |
| `usart.h` | USART1/2, any baud/parity/stop bits, polling + timeouts, interrupt and DMA transfers (L4 and G4 DMA routing) |
| `spi.h` | `spi_configure` master/slave + mode + prescaler, full-duplex DMA |
| `timer.h` | output polarity, master/slave synchronisation, indirect capture (PWM input), repetition counter, CCR address for DMA, TIM1/TIM8 capture interrupts |
| `flash.h` | page ranges, mass erase, fast programming (RAM routines), write protection |
| `wwdg.h` | `wwdg_start/refresh/enable_early_wakeup` (G4 prescaler up to /128) |
| `iwdg.h`, `pwr.h`, `rtc.h` | new: independent watchdog with window, Stop 0/1 and Standby, LSI RTC wakeup timer |
| `cordic.h`, `fmac.h` | new, G4 only: CORDIC (polled, DMA), FMAC (FIR/IIR, DMA, software model) |

## Differences from ST's examples

- **Flash user area = pages 32..63** (ST: pages 4..63): a stefi-lib
  image with `printf` is ~28 KB, more than the 8 KB below page 4.
- **FLASH_FastProgram** page-erases the user area; ST mass-erases the
  bank and runs the whole program from SRAM.
- **FMAC**: the reference output is computed with `fmac_model_iir()`
  (bit-exact model, checked against ST's table) from a generated test
  signal instead of ST's input/reference tables.
- **CORDIC**: reference sines come from `sin()` at run time, not a table.
- **TIM_DMA**: TIM3 has no repetition counter (ST's text mentions one);
  the duty cycle changes every period.
- **TIM_InputCapture / TIM_PWMInput** print the measured values on the
  ST-Link VCP once a second instead of only storing them in variables.
- **PWR examples** do not put unused GPIOs into analog mode, so the
  measured Stop/Standby current is somewhat higher than ST's figures.

## Testing status

All 25 executables build without warnings (flash images 27–36 KB). The
FMAC model is unit-tested on the host (`sh tests/host/run.sh`). None of
the examples has been run on a NUCLEO-G431KB yet.
