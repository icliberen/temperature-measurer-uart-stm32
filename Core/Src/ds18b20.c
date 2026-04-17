#include "ds18b20.h"

#define OW_CMD_SKIP_ROM     0xCC
#define OW_CMD_CONVERT_T    0x44
#define OW_CMD_READ_SCRATCH 0xBE

static uint32_t cycles_per_us = 168;

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * cycles_per_us;
    while ((DWT->CYCCNT - start) < ticks) { }
}

static void ow_pin_output_low(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = DS18B20_GPIO_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &g);
    HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_RESET);
}

static void ow_pin_release(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = DS18B20_GPIO_PIN;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_PORT, &g);
}

static uint8_t ow_pin_read(void)
{
    return HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN) == GPIO_PIN_SET ? 1 : 0;
}

static uint8_t ow_reset(void)
{
    ow_pin_output_low();
    delay_us(480);
    ow_pin_release();
    delay_us(70);
    uint8_t presence = (ow_pin_read() == 0) ? 1 : 0;
    delay_us(410);
    return presence;
}

static void ow_write_bit(uint8_t bit)
{
    ow_pin_output_low();
    if (bit) {
        delay_us(6);
        ow_pin_release();
        delay_us(64);
    } else {
        delay_us(60);
        ow_pin_release();
        delay_us(10);
    }
}

static uint8_t ow_read_bit(void)
{
    uint8_t b;
    ow_pin_output_low();
    delay_us(3);
    ow_pin_release();
    delay_us(10);
    b = ow_pin_read();
    delay_us(53);
    return b;
}

static void ow_write_byte(uint8_t v)
{
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_bit(v & 0x01);
        v >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t v = 0;
    for (uint8_t i = 0; i < 8; i++) {
        v >>= 1;
        if (ow_read_bit()) v |= 0x80;
    }
    return v;
}

static uint8_t ow_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ b) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            b >>= 1;
        }
    }
    return crc;
}

void DS18B20_Init(void)
{
    if (DS18B20_GPIO_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (DS18B20_GPIO_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (DS18B20_GPIO_PORT == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (DS18B20_GPIO_PORT == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (DS18B20_GPIO_PORT == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();

    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
    if (cycles_per_us == 0) cycles_per_us = 1;

    ow_pin_release();
}

HAL_StatusTypeDef DS18B20_ReadTemperature(float *out_c)
{
    if (!out_c) return HAL_ERROR;

    if (!ow_reset()) return HAL_ERROR;
    ow_write_byte(OW_CMD_SKIP_ROM);
    ow_write_byte(OW_CMD_CONVERT_T);

    HAL_Delay(750);

    if (!ow_reset()) return HAL_ERROR;
    ow_write_byte(OW_CMD_SKIP_ROM);
    ow_write_byte(OW_CMD_READ_SCRATCH);

    uint8_t scratch[9];
    for (uint8_t i = 0; i < 9; i++) scratch[i] = ow_read_byte();

    if (ow_crc8(scratch, 8) != scratch[8]) return HAL_ERROR;

    int16_t raw = (int16_t)((uint16_t)scratch[0] | ((uint16_t)scratch[1] << 8));
    *out_c = (float)raw / 16.0f;
    return HAL_OK;
}
