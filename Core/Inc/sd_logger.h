#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/*
 * SD card logger over SPI2 (PB13 SCK, PB14 MISO, PB15 MOSI, PB12 CS).
 * Requires FatFS middleware enabled in CubeMX (User-defined, SPI mode).
 */

uint8_t SD_Logger_Init(void);
void    SD_Logger_Append(uint32_t ts_ms, float t_int, float t_ext);
void    SD_Logger_Sync(void);
uint8_t SD_Logger_IsReady(void);

#endif
