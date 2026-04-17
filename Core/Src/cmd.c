#include "cmd.h"
#include "sd_logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern volatile uint8_t logging_enabled;
extern TIM_HandleTypeDef htim2;

#define CMD_BUF_LEN 64

static UART_HandleTypeDef *cmd_uart = NULL;
uint8_t         cmd_rx_byte;
static char     line_buf[CMD_BUF_LEN];
static uint16_t line_len = 0;
static volatile uint8_t line_ready = 0;
static char     pending[CMD_BUF_LEN];
static uint16_t pending_len = 0;
static uint32_t current_rate_hz = 10;

static void cmd_send(const char *s)
{
    if (!cmd_uart) return;
    HAL_UART_Transmit(cmd_uart, (uint8_t*)s, (uint16_t)strlen(s), 100);
}

static void cmd_send_line(const char *s)
{
    cmd_send(s);
    cmd_send("\r\n");
}

static int ieq(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

static HAL_StatusTypeDef apply_rate(uint32_t hz)
{
    if (hz < 1 || hz > 50) return HAL_ERROR;

    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t tim_clk = pclk1;
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0) tim_clk = pclk1 * 2;

    uint32_t prescaler = (tim_clk / 10000U);
    if (prescaler == 0) prescaler = 1;
    uint32_t period = (tim_clk / prescaler) / hz;
    if (period == 0) period = 1;

    HAL_TIM_Base_Stop_IT(&htim2);
    __HAL_TIM_SET_PRESCALER(&htim2, prescaler - 1);
    __HAL_TIM_SET_AUTORELOAD(&htim2, period - 1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    if (logging_enabled) {
        HAL_TIM_Base_Start_IT(&htim2);
    }
    current_rate_hz = hz;
    return HAL_OK;
}

static void handle_line(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = 0;
    }
    if (*s == 0) return;

    if (ieq(s, "start")) {
        logging_enabled = 1;
        cmd_send_line("OK LOGGING=ON");
        return;
    }
    if (ieq(s, "stop")) {
        logging_enabled = 0;
        cmd_send_line("OK LOGGING=OFF");
        return;
    }
    if (ieq(s, "help")) {
        cmd_send_line("CMDS: start | stop | rate <1..50> | dump | help");
        return;
    }
    if (ieq(s, "dump")) {
        char out[96];
        snprintf(out, sizeof(out), "CFG RATE=%lu LOGGING=%s SD=%s",
                 (unsigned long)current_rate_hz,
                 logging_enabled ? "on" : "off",
                 SD_Logger_IsReady() ? "ready" : "na");
        cmd_send_line(out);
        return;
    }
    if (strncmp(s, "rate", 4) == 0 && (s[4] == ' ' || s[4] == '\t')) {
        char *p = s + 4;
        while (*p == ' ' || *p == '\t') p++;
        long hz = strtol(p, NULL, 10);
        if (apply_rate((uint32_t)hz) == HAL_OK) {
            char out[32];
            snprintf(out, sizeof(out), "OK RATE=%ld", hz);
            cmd_send_line(out);
        } else {
            cmd_send_line("ERR rate out of range");
        }
        return;
    }
    cmd_send_line("ERR unknown");
}

void Cmd_Init(UART_HandleTypeDef *huart)
{
    cmd_uart = huart;
    line_len = 0;
    line_ready = 0;
    pending_len = 0;
    HAL_UART_Receive_IT(cmd_uart, &cmd_rx_byte, 1);
}

void Cmd_OnRxByte(uint8_t b)
{
    if (b == '\r' || b == '\n') {
        if (line_len > 0 && !line_ready) {
            memcpy(pending, line_buf, line_len);
            pending[line_len] = 0;
            pending_len = line_len;
            line_ready = 1;
        }
        line_len = 0;
    } else if (line_len < CMD_BUF_LEN - 1) {
        line_buf[line_len++] = (char)b;
    } else {
        line_len = 0;
    }
    if (cmd_uart) HAL_UART_Receive_IT(cmd_uart, &cmd_rx_byte, 1);
}

void Cmd_Process(void)
{
    if (!line_ready) return;
    handle_line(pending);
    pending_len = 0;
    line_ready = 0;
}
