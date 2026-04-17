#ifndef DS18B20_H
#define DS18B20_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/*
 * DS18B20 1-Wire temperature sensor driver (bit-bang).
 *
 * Default data line: PC1. Override by defining DS18B20_GPIO_PORT and
 * DS18B20_GPIO_PIN before including this header (or via -D build flags).
 *
 * A 4.7k pull-up between the data line and 3.3V is required.
 */

#ifndef DS18B20_GPIO_PORT
#define DS18B20_GPIO_PORT GPIOC
#endif

#ifndef DS18B20_GPIO_PIN
#define DS18B20_GPIO_PIN  GPIO_PIN_1
#endif

void              DS18B20_Init(void);
HAL_StatusTypeDef DS18B20_ReadTemperature(float *out_c);

#endif
