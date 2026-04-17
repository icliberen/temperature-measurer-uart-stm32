# 🌡️ Temperature Measurer — UART STM32

Real-time internal temperature measurement on the **STM32F407VGT6** (Discovery board) with UART data logging. The MCU's built-in temperature sensor is sampled via ADC + DMA, and calibrated readings are streamed over UART at 115200 baud.

## Features

- **Internal temperature sensor** — reads the on-chip temperature sensor (`ADC_CHANNEL_TEMPSENSOR`) and internal voltage reference (`ADC_CHANNEL_VREFINT`)
- **Factory-calibrated conversion** — uses ST's factory calibration values (30 °C / 110 °C) stored in ROM for accurate °C output
- **DMA double-buffering** — circular DMA with half-transfer / transfer-complete interrupts for zero-copy, glitch-free sampling
- **Timer-triggered ADC** — TIM2 TRGO triggers ADC conversions at a steady 10 Hz rate (configurable via prescaler/period)
- **UART output** — formatted `TEMP_RAW=xxxx VREF_RAW=xxxx T=xx.xx C` lines transmitted over USART2 (PA2/PA3) at **115200 8N1**
- **Button-toggled logging** — press the blue user button (PA0) to start/stop data acquisition at runtime
- **PWM LED indicator** — green LED (PD12) brightness reflects logging state: dim when idle, bright when active
- **Low-power idle** — enters WFI (Wait For Interrupt) sleep between samples to save energy

## Hardware

| Component | Detail |
|---|---|
| **Board** | [STM32F407G-DISC1](https://www.st.com/en/evaluation-tools/stm32f4discovery.html) |
| **MCU** | STM32F407VGT6 — ARM Cortex-M4, 168 MHz, 1 MB Flash, 192 KB RAM |
| **Clock** | HSE 8 MHz → PLL → 168 MHz SYSCLK |
| **UART** | USART2 — PA2 (TX) / PA3 (RX), directly routed to ST-Link VCP |
| **ADC** | ADC1 — 12-bit, DMA2 Stream0, timer-triggered |
| **Timers** | TIM2 (ADC trigger, 10 Hz) · TIM4 CH1 (PWM on PD12) |
| **Button** | PA0 — User button, EXTI rising edge |
| **LED** | PD12 — Green LED, PWM-controlled brightness |

## Project Structure

```
├── Core/
│   ├── Inc/
│   │   ├── main.h                  # Pin/peripheral defines
│   │   ├── logger.h                # Logger module API
│   │   ├── stm32f4xx_hal_conf.h    # HAL configuration
│   │   └── stm32f4xx_it.h         # Interrupt handler prototypes
│   └── Src/
│       ├── main.c                  # Application entry point & peripheral init
│       ├── logger.c                # Temperature conversion & UART transmit
│       ├── stm32f4xx_hal_msp.c     # MSP initialization (clocks, GPIOs for peripherals)
│       ├── stm32f4xx_it.c          # Interrupt handlers
│       ├── syscalls.c              # Newlib system calls
│       ├── sysmem.c                # Heap management
│       └── system_stm32f4xx.c      # CMSIS system initialization
├── Drivers/                        # STM32 HAL & CMSIS drivers
├── Middlewares/                    # ST USB Host middleware
├── USB_HOST/                       # USB Host application files
├── datalogger.ioc                  # STM32CubeMX project file
├── STM32F407VGTX_FLASH.ld          # Linker script (flash)
├── STM32F407VGTX_RAM.ld            # Linker script (RAM)
├── .cproject                       # STM32CubeIDE CDT project
├── .project                        # Eclipse project descriptor
└── README.md
```

## Getting Started

### Prerequisites

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (v1.13+ recommended)
- STM32F407G-DISC1 board connected via USB (ST-Link)
- A serial terminal (PuTTY, Tera Term, `screen`, etc.)

### Build & Flash

1. **Clone** the repository:
   ```bash
   git clone https://github.com/icliberen/temperature-measurer-uart-stm32.git
   ```
2. **Open** STM32CubeIDE → *File → Import → Existing Projects into Workspace* → select the cloned folder.
3. **Build** the project (`Ctrl+B` or *Project → Build All*).
4. **Flash** to the board (`F11` or *Run → Debug*).

### View Output

1. Open a serial terminal on the ST-Link VCP port:
   - **Baud rate:** 115200
   - **Data bits:** 8
   - **Parity:** None
   - **Stop bits:** 1
2. Press the **blue user button** on the Discovery board to start logging.
3. You will see output like:
   ```
   DATALOGGER BOOT
   TEMP_RAW=942 VREF_RAW=1526  T=27.34 C
   TEMP_RAW=943 VREF_RAW=1525  T=27.48 C
   ...
   ```
4. Press the button again to stop logging.

## How It Works

### Temperature Conversion

The firmware uses ST's factory calibration data stored at fixed ROM addresses:

| Constant | Address | Description |
|---|---|---|
| `VREFINT_CAL` | `0x1FFF7A2A` | VREFINT ADC reading at 3.3 V, 30 °C |
| `TS_CAL1` | `0x1FFF7A2C` | Temp sensor ADC reading at 30 °C |
| `TS_CAL2` | `0x1FFF7A2E` | Temp sensor ADC reading at 110 °C |

**Conversion formula:**

```
VDDA = 3.3 × VREFINT_CAL / VREFINT_RAW
temp_normalized = TEMP_RAW × (VDDA / 3.3)
T(°C) = 30 + (temp_normalized − TS_CAL1) × 80 / (TS_CAL2 − TS_CAL1)
```

### Data Flow

```
TIM2 TRGO (10 Hz)
    │
    ▼
ADC1 (TEMPSENSOR + VREFINT, 2-channel scan)
    │
    ▼
DMA2 Stream0 (circular, half/full callbacks)
    │
    ▼
logger.c → average + calibrate → UART TX
```

## Configuration

| Parameter | Location | Default | Description |
|---|---|---|---|
| Sampling rate | `MX_TIM2_Init()` | 10 Hz | Change `Prescaler` and `Period` |
| Baud rate | `MX_USART2_UART_Init()` | 115200 | UART baud rate |
| Buffer size | `#define ADC_BUF_SIZE` | 20 | DMA buffer (samples × 2 channels) |
| LED idle brightness | `main.c` | 10% | `Set_LED_Duty(10)` |
| LED active brightness | `main.c` | 80% | `Set_LED_Duty(80)` |

## CubeMX Regeneration

The project includes `datalogger.ioc`. You can open it in STM32CubeMX or CubeIDE's IOC editor to modify pin/peripheral settings. User code is preserved inside `/* USER CODE BEGIN */` / `/* USER CODE END */` blocks.

## Enhancements

The firmware ships with four optional add-ons that can be enabled
independently. All hooks live in `main.c` / `logger.c` and the existing
`TEMP_RAW=... T=xx.xx C` field is preserved for backwards compatibility.

### DS18B20 External Sensor

A 1-Wire bit-bang driver on **PC1** (override via `DS18B20_GPIO_PORT` /
`DS18B20_GPIO_PIN` in `ds18b20.h`). Requires a 4.7 k pull-up between the
data line and 3.3 V. DWT cycles are used for microsecond timing, so the
sensor does not consume an extra TIM.

Source: `Core/Inc/ds18b20.h`, `Core/Src/ds18b20.c`.

API:

```c
void              DS18B20_Init(void);
HAL_StatusTypeDef DS18B20_ReadTemperature(float *out_c);
```

Enable by calling `DS18B20_Init()` after `MX_GPIO_Init()` (already wired
in `main.c`). Disable by commenting out the call — the logger then emits
`T_EXT=NA`. Example line with the sensor attached:

```
TEMP_RAW=942 VREF_RAW=1526  T_INT=27.34 C T=27.34 C T_EXT=24.18 C
```

### SD Card Logging

CSV logger on **SPI2** with FatFS.

| Signal | Pin  |
|--------|------|
| SCK    | PB13 |
| MISO   | PB14 |
| MOSI   | PB15 |
| CS     | PB12 (GPIO output) |

Required CubeMX steps:

1. **Middleware -> FATFS** -> Mode: **User-defined**.
2. **Connectivity -> SPI2** -> Full-Duplex Master, map pins as above.
3. Add CS as a regular GPIO output on **PB12** (initial level HIGH).
4. In `user_diskio.c`, implement the `disk_read` / `disk_write` / `disk_status`
   stubs using `HAL_SPI_TransmitReceive` and toggle PB12 for chip select.
5. Regenerate code (this makes `ff.h` available on the include path).

CSV format (header written once):

```
timestamp_ms,t_internal_c,t_external_c
12043,27.34,24.18
12543,27.35,24.19
```

If FatFS is not yet enabled, `SD_Logger_Init()` returns non-zero and the
`SD_Logger_Append` / `SD_Logger_Sync` calls are no-ops.

Source: `Core/Inc/sd_logger.h`, `Core/Src/sd_logger.c`.

### Command Interface

Interactive control over the same USART2 link (115200 8N1), newline- or
CR-terminated. Implemented via `HAL_UART_Receive_IT` with 1-byte rearm.

| Command       | Example   | Response              |
|---------------|-----------|-----------------------|
| `start`       | `start`   | `OK LOGGING=ON`       |
| `stop`        | `stop`    | `OK LOGGING=OFF`      |
| `rate <hz>`   | `rate 25` | `OK RATE=25`          |
| `dump`        | `dump`    | `CFG RATE=10 LOGGING=on SD=ready` |
| `help`        | `help`    | list of commands      |
| (unknown)     | `foo`     | `ERR unknown`         |

`rate` accepts an integer **1..50 Hz** and reconfigures TIM2's prescaler
and auto-reload from the APB1 timer clock.

Source: `Core/Inc/cmd.h`, `Core/Src/cmd.c`.

### Host Script

`tools/live_plot.py` parses the stream and shows a rolling matplotlib
plot of `T_INT` and (if present) `T_EXT`. Install & run:

```
cd tools
pip install -r requirements.txt
python live_plot.py --port COM5
```

Arguments: `--port`, `--baud` (default 115200), `--window` (default 200).
On Ctrl+C the script writes `capture.csv` with columns
`t_s,t_internal_c,t_external_c`.

See `tools/README.md` for details.

## License

This project is licensed under the [MIT License](LICENSE).
