#ifndef LOGGER_H
#define LOGGER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

float LOGGER_ConvertTemp_Cal(uint16_t raw_temp, uint16_t raw_vref);
void  LOGGER_SendBlock(UART_HandleTypeDef *huart, uint16_t *buf, uint16_t len);

#endif
