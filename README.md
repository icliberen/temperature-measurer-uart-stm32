# 🌡️ TemperatureMeasurer — UART STM32

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

## License

This project is licensed under the [MIT License](LICENSE).
