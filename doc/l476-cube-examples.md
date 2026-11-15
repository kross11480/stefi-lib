# STM32CubeL4 examples on stefi-lib

ST ships 72 register-level demos for the NUCLEO-L476RG in
[STM32CubeL4 / Projects/NUCLEO-L476RG/Examples](https://github.com/STMicroelectronics/STM32CubeL4/tree/master/Projects/NUCLEO-L476RG/Examples).
[`examples/12-NucleoL476RG`](https://github.com/kross11480/stefi-lib/tree/main/examples/12-NucleoL476RG)
re-implements them on stefi-lib (69 source folders, 92 executables)
with the same pins, timings and LED2 signalling, and no ST HAL.

```sh
cmake -B cmake-build-l4 -DSTEFI_BOARD=nucleo_l476rg
cmake --build cmake-build-l4
openocd -f configs/st_nucleo_l4.cfg \
        -c "program cmake-build-l4/examples/12-NucleoL476RG/l4_gpio_iotoggle verify reset exit"
```

Board `nucleo_l476rg` is the bare NUCLEO-64 (MB1136): LD2 on PA5 (`LED2`),
B1 on PC13 (`BUTTON_USER`), the ST-Link VCP on USART2 and the core at 80 MHz
(PLL from MSI). Use `nucleo_l476` for the STefi adapter board.

The examples follow the style of the other stefi-lib examples. Each one calls
`board_init()` and uses the components (`led_*`, `button_is_pressed`,
`button_interrupt_init`, `htimer_*`) where ST used a BSP or HAL call. A
peripheral without a component is driven through `<libstefi/...>`. Small
helpers (`led_error_forever`, `wait_for_button`) are local to each file.

LED2 conventions (from ST's examples): **on** = passed, **blinking 1 s** =
wrong result, **100 ms on / 2 s off** = a peripheral operation failed.

ST's master/slave, transmitter/receiver, Stop 1/Stop 2 and
`WRITE_PROTECTION_*` project variants are separate executables here, built
from one `main.c` with a compile definition (see `CMakeLists.txt`).

## Examples and the drivers they use

| ST example | stefi executable(s) | stefi APIs |
|---|---|---|
| COMP_Interrupt | `l4_comp_interrupt` | `comp.h` (new): window/threshold, EXTI wakeup; `pwr.h` Stop 2 |
| CRC_Bytes_Stream_7bit_CRC | `l4_crc_bytes_stream_7bit_crc` | `crc.h` (new): polynomial size, init, I/O reversal, byte/half-word feed; `crc_sw` model |
| CRC_Data_Reversing_16bit_CRC | `l4_crc_data_reversing_16bit_crc` | `crc.h` (new): polynomial size, init, I/O reversal, byte/half-word feed; `crc_sw` model |
| CRC_Example | `l4_crc_example` | `crc.h` (new): polynomial size, init, I/O reversal, byte/half-word feed; `crc_sw` model |
| CRC_UserDefinedPolynomial | `l4_crc_user_defined_polynomial` | `crc.h` (new): polynomial size, init, I/O reversal, byte/half-word feed; `crc_sw` model |
| CORTEXM_MPU | `l4_cortexm_mpu`, `l4_cortexm_mpu_fault` | `cortex.h` (new): `mpu_set_region`, `cortex_on_memmanage_fault` |
| CORTEXM_ModePrivilege | `l4_cortexm_mode_privilege` | `cortex.h`: process stack, `cortex_drop_privilege`, SVC to regain it |
| CORTEXM_SysTick | `l4_cortexm_systick` | `cortex.h` SysTick reload, `led.h` |
| DAC_SignalsGeneration | `l4_dac_signals_generation` | `dac.h` (new): 8/12-bit, triangle/noise, TIM6 trigger |
| DAC_SimpleConversion | `l4_dac_simple_conversion` | `dac.h` (new): 8/12-bit, triangle/noise, TIM6 trigger |
| FIREWALL_VolatileData_Executable | `l4_firewall_volatile_data_executable` | `firewall.h` (new): segments, call gate, volatile-data shared/executable; `rtc.h` backup registers; `firewall.ld` |
| FIREWALL_VolatileData_Shared | `l4_firewall_volatile_data_shared` | `firewall.h` (new): segments, call gate, volatile-data shared/executable; `rtc.h` backup registers; `firewall.ld` |
| FLASH_DualBoot | `l4_flash_dual_boot_bank1`, `l4_flash_dual_boot_bank2` | `flash.h`: `flash_set_dual_boot`, `flash_boot_bank` |
| FLASH_EraseProgram | `l4_flash_erase_program` | `flash.h`: `flash_erase_pages`, `flash_write` |
| FLASH_FastProgram | `l4_flash_fast_program` | `flash.h`: `flash_mass_erase_bank`, `flash_fast_program_row` |
| FLASH_WriteProtection | `l4_flash_write_protection_program`, `l4_flash_write_protection_enable`, `l4_flash_write_protection_disable` | `flash.h`: WRP areas 1A/1B/2A/2B (option bytes) |
| GPIO_Demo | `l4_gpio_demo` | `led.h`, `button.h` (debounced EXTI) |
| GPIO_EXTI | `l4_gpio_exti` | `led.h`, `button.h` (debounced EXTI) |
| GPIO_IOToggle | `l4_gpio_iotoggle` | `led.h`, `button.h` (debounced EXTI) |
| HAL_RegisterCallbacks_TIM | `l4_hal_register_callbacks_tim` | `hardware_timer.h`: `htimer_init_periodic_ms` callback |
| HAL_TimeBase_TIM | `l4_hal_time_base_tim` | `hardware_timer.h`: TIM6 1 ms tick, `htimer_stop/start` |
| I2C_TwoBoards_AdvComIT | `l4_i2c_twoboards_adv_it_master`, `l4_i2c_twoboards_adv_it_slave` | `i2c.h`: `i2c_bus_init`, master/slave transfers |
| I2C_TwoBoards_ComDMA | `l4_i2c_twoboards_dma_master`, `l4_i2c_twoboards_dma_slave` | `i2c.h`: `i2c_master_transfer_dma`, `i2c_slave_*_dma`, 10-bit address |
| I2C_TwoBoards_ComIT | `l4_i2c_twoboards_it_master`, `l4_i2c_twoboards_it_slave` | `i2c.h`: `i2c_bus_init`, master/slave transfers |
| I2C_TwoBoards_ComPolling | `l4_i2c_twoboards_polling_master`, `l4_i2c_twoboards_polling_slave` | `i2c.h`: `i2c_bus_init`, master/slave transfers |
| I2C_TwoBoards_RestartAdvComIT | `l4_i2c_twoboards_restart_adv_it_master`, `l4_i2c_twoboards_restart_adv_it_slave` | `i2c.h`: `i2c_slave_listen` (address + complete callbacks), repeated start |
| I2C_TwoBoards_RestartComIT | `l4_i2c_twoboards_restart_it_master`, `l4_i2c_twoboards_restart_it_slave` | `i2c.h`: `i2c_master_transfer_it` write + read with repeated start |
| I2C_WakeUpFromStop / _WakeUpFromStop2 | `l4_i2c_wakeup_from_stop_stop1_master`, `l4_i2c_wakeup_from_stop_stop1_slave`, `l4_i2c_wakeup_from_stop_stop2_master`, `l4_i2c_wakeup_from_stop_stop2_slave` | `i2c.h` (new transfer engine): HSI16 kernel clock, `i2c_enable_wakeup`; `pwr.h` Stop 1/2 |
| IWDG_Reset | `l4_iwdg_reset` | `iwdg.h`, `system.h` reset flags, `button.h` |
| IWDG_WindowMode | `l4_iwdg_window_mode` | `iwdg.h`, `system.h` reset flags, `button.h` |
| LPTIM_PWMExternalClock | `l4_lptim_pwm_external_clock` | `lptim.h` (new): PWM / external clock / pulse counter; `pwr.h` Stop |
| LPTIM_PulseCounter | `l4_lptim_pulse_counter` | `lptim.h` (new): PWM / external clock / pulse counter; `pwr.h` Stop |
| OPAMP_PGA | `l4_opamp_pga` | `opamp.h` (new): PGA gain, `dac.h` input |
| PWR_LPRUN | `l4_pwr_lprun` | `pwr.h`: `pwr_lprun_enter/exit`, `system_init_l4_msi` |
| PWR_LPRUN_SRAM1 | `l4_pwr_lprun_sram1` | `pwr.h` LP run, code in SRAM1 |
| PWR_LPSLEEP | `l4_pwr_lpsleep` | `pwr.h`, `button.h` wakeup |
| PWR_ModesSelection | `l4_pwr_modes_selection` | `pwr.h` all modes, `rtc.h` wakeup timer, `uart.h` menu |
| PWR_SHUTDOWN | `l4_pwr_shutdown` | `pwr.h`: `pwr_enter_shutdown`, wakeup pin |
| PWR_SLEEP | `l4_pwr_sleep` | `pwr.h`, `button.h` wakeup |
| PWR_STANDBY | `l4_pwr_standby` | `pwr.h`, `button.h` wakeup |
| PWR_STANDBY_RTC | `l4_pwr_standby_rtc` | `pwr.h` Standby, `rtc.h` wakeup timer |
| PWR_STOP1 / PWR_STOP2 | `l4_pwr_stop1`, `l4_pwr_stop2` | `pwr.h`, `button.h` wakeup |
| PWR_STOP1_RTC / PWR_STOP2_RTC | `l4_pwr_stop1_rtc`, `l4_pwr_stop2_rtc` | `pwr.h` Stop 1/2, `rtc.h` wakeup timer |
| RCC_ClockConfig | `l4_rcc_clock_config` | `system.h`: `system_init_l4` (HSE/MSI PLL), `system_mco` |
| RTC_Alarm | `l4_rtc_alarm` | `rtc.h`: `rtc_set_datetime`, `rtc_alarm_a_set` |
| RTC_LSI | `l4_rtc_lsi` | `rtc.h` alarm, `system_measure_lsi_hz` (TIM16 capture) |
| RTC_Tamper | `l4_rtc_tamper` | `rtc.h`: backup registers, `rtc_tamper1_enable` |
| RTC_TimeStamp | `l4_rtc_time_stamp` | `rtc.h`: `rtc_timestamp_enable/get` |
| SPI_FullDuplex_ComDMA | `l4_spi_fullduplex_dma_master`, `l4_spi_fullduplex_dma_slave` | `spi.h`: `spi_transfer_dma` |
| SPI_FullDuplex_ComIT | `l4_spi_fullduplex_it_master`, `l4_spi_fullduplex_it_slave` | `spi.h`: `spi_transfer_it` |
| SPI_FullDuplex_ComPolling | `l4_spi_fullduplex_polling_master`, `l4_spi_fullduplex_polling_slave` | `spi.h`: `spi_transfer_buffer` |
| SPI_HalfDuplex_ComPolling | `l4_spi_halfduplex_polling_master`, `l4_spi_halfduplex_polling_slave` | `spi.h`: `spi_configure_half_duplex`, `spi_half_duplex_direction` |
| TIM_DMA | `l4_tim_dma` | `timer.h` CCR DMA requests; `dma_channel.h` circular |
| TIM_DMABurst | `l4_tim_dma_burst` | `timer.h`: `timer_dma_burst`; `dma_channel.h` |
| TIM_ExtTriggerSynchro | `l4_tim_ext_trigger_synchro` | `timer.h` master/slave gated + trigger |
| TIM_InputCapture | `l4_tim_input_capture` | `timer.h` input capture / PWM |
| TIM_OCActive | `l4_tim_oc_active` | `timer.h`: `timer_set_oc_mode` |
| TIM_OCInactive | `l4_tim_oc_inactive` | `timer.h`: `timer_set_oc_mode` |
| TIM_OCToggle | `l4_tim_oc_toggle` | `timer.h`: `timer_set_oc_mode` |
| TIM_OnePulse | `l4_tim_one_pulse` | `timer.h` one-pulse mode, slave trigger |
| TIM_PWMInput | `l4_tim_pwm_input` | `timer.h` input capture / PWM |
| TIM_PWMOutput | `l4_tim_pwm_output` | `timer.h` input capture / PWM |
| TIM_TimeBase | `l4_tim_time_base` | `hardware_timer.h`: `htimer_init_periodic_ms` |
| UART_ReceptionToIdle_CircularDMA | `l4_uart_reception_to_idle_circular_dma` | `usart.h`: `usart_read_to_idle_circular` |
| UART_TwoBoards_ComDMA | `l4_uart_twoboards_dma_tx`, `l4_uart_twoboards_dma_rx` | `usart.h` polling / IT / DMA, USART1 |
| UART_TwoBoards_ComIT | `l4_uart_twoboards_it_tx`, `l4_uart_twoboards_it_rx` | `usart.h` polling / IT / DMA, USART1 |
| UART_TwoBoards_ComPolling | `l4_uart_twoboards_polling_tx`, `l4_uart_twoboards_polling_rx` | `usart.h` polling / IT / DMA, USART1 |
| UART_WakeUpFromStop | `l4_uart_wakeup_from_stop_stop1`, `l4_uart_wakeup_from_stop_stimulus` | `usart.h`: `usart_enable_wakeup`, `usart_wakeup_address`; `pwr.h` Stop 1 |
| WWDG_Example | `l4_wwdg_example` | `wwdg.h` |

## New and extended drivers (STM32L4 / STM32G4)

Register maps are in `src/internal/*_internal.h` as structs, the same as the
existing drivers (`CRC_t`, `COMP_t`, `DAC_t`, `OPAMP_t`, `LPTIM_t`,
`FIREWALL_t`, `RTC_L4_t`/`RTC_G4_t`, `MPU_typeDef`, ...).

| | |
|---|---|
| `crc.h`, `comp.h`, `dac.h`, `opamp.h`, `lptim.h`, `firewall.h` | new peripheral drivers |
| `i2c.h` | I2C v2 transfer engine: master/slave, 7/10-bit, polling/IT/DMA, listen mode, wakeup from Stop |
| `cortex.h` | MPU regions, MemManage hook, process stack / unprivileged mode, SVC |
| `rtc.h` | calendar, alarm A, backup registers, tamper 1, timestamp (L4 and G4 register layouts) |
| `pwr.h` | Sleep, LP run, LP sleep, Stop 0/1/2, Standby, Shutdown, wakeup pins, voltage range |
| `system.h` | `system_init_l4(src, hz)`, `system_init_l4_msi(range)`, MCO, LSI measurement |
| `flash.h` | dual bank: bank mass erase, WRP areas 2A/2B, dual boot (BFB2) |
| `timer.h` | output-compare modes, complementary outputs, CC interrupts, DMA burst |
| `spi.h` | SPI2/3, buffer and interrupt transfers, half duplex |
| `usart.h` | reception to idle (circular DMA), wakeup from Stop by address |
| `hardware_timer.h` (component) | clock-aware: 10 kHz tick from `system_get_core_clock()`, any core clock |

## Differences from ST's examples

- **Flash areas sit above the stefi-lib image** (about 28 KB with `printf`).
  EraseProgram uses pages 32..255, FastProgram uses bank 2 (which it
  mass-erases), and WriteProtection protects pages 64..68.
- **FIREWALL** segments are placed by a `firewall.ld` per example, added to
  the board linker script with `INSERT`. In VolatileData_Shared the SRAM data
  is at 0x20017000; ST puts it at 0x20000000, where stefi-lib's `.data` lives.
  Progress across the provoked firewall resets is kept in an RTC backup
  register.
- **HAL_TimeBase_TIM / HAL_RegisterCallbacks_TIM / TIM_TimeBase** use the
  `hardware_timer` component (TIM6 / TIM3) instead of HAL tick callbacks.
- **PWR_LPRUN** polls B1 in LP run: the debounce timer behind
  `button_interrupt_init` is too slow at 100 kHz.
- **CRC**: all results are checked against ST's expected values and against
  the software model `crc_sw()`.
- **PWR examples** do not put unused GPIOs into analog mode, so the
  measured currents are somewhat higher than ST's figures.

## Testing status

All 92 executables build without warnings. Flash images are 27–36 KB.
The CRC values and the firewall section placement (call gate at segment
start + 4) were checked against ST's sources and the linker map. None of the
examples has been run on a NUCLEO-L476RG yet.
