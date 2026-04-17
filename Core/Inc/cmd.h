#ifndef CMD_H
#define CMD_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

void Cmd_Init(UART_HandleTypeDef *huart);
void Cmd_OnRxByte(uint8_t b);
void Cmd_Process(void);

#endif
