#include "logger.h"
#include <stdio.h>

#ifndef VREFINT_CAL_ADDR
#define VREFINT_CAL_ADDR ((uint16_t*)0x1FFF7A2A)
#endif
#ifndef TEMP30_CAL_ADDR
#define TEMP30_CAL_ADDR  ((uint16_t*)0x1FFF7A2C)
#endif
#ifndef TEMP110_CAL_ADDR
#define TEMP110_CAL_ADDR ((uint16_t*)0x1FFF7A2E)
#endif

float LOGGER_ConvertTemp_Cal(uint16_t raw_temp, uint16_t raw_vref)
{
    uint16_t vref_cal = *VREFINT_CAL_ADDR;     // VREFINT @ 3.3V
    uint16_t t30_cal  = *TEMP30_CAL_ADDR;      // TS @ 30°C
    uint16_t t110_cal = *TEMP110_CAL_ADDR;     // TS @ 110°C

    if (raw_vref == 0) return -999.0f;

    // Estimate VDDA
    float vdda = 3.3f * (float)vref_cal / (float)raw_vref;

    // Normalize temp raw to 3.3V domain (calibration assumes VDDA=3.3V)
    float raw_temp_3v3 = (float)raw_temp * (vdda / 3.3f);

    // Linear interpolation between 30°C and 110°C
    return 30.0f + (raw_temp_3v3 - (float)t30_cal) * (110.0f - 30.0f) / ((float)t110_cal - (float)t30_cal);
}

void LOGGER_SendBlock(UART_HandleTypeDef *huart, uint16_t *buf, uint16_t len)
{
    if (len < 2) return;

    // Expect interleaved data: TEMP, VREF, TEMP, VREF...
    uint32_t sum_temp = 0, sum_vref = 0;
    uint16_t pairs = len / 2;

    for (uint16_t i = 0; i < pairs; i++)
    {
        sum_temp += buf[2*i + 0];
        sum_vref += buf[2*i + 1];
    }

    uint16_t raw_temp = (uint16_t)(sum_temp / pairs);
    uint16_t raw_vref = (uint16_t)(sum_vref / pairs);

    float t = LOGGER_ConvertTemp_Cal(raw_temp, raw_vref);

    char msg[96];
    int n = snprintf(msg, sizeof(msg),
                     "TEMP_RAW=%u VREF_RAW=%u  T=%.2f C\r\n",
                     raw_temp, raw_vref, t);

    HAL_UART_Transmit(huart, (uint8_t*)msg, (uint16_t)n, 100);
}
