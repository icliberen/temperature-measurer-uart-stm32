/*
 * SD card logger — SPI2 + FatFS.
 *
 * IMPORTANT: This file will NOT compile out of the box. You must enable
 * FatFS in STM32CubeMX first:
 *   1. Pinout & Configuration -> Middleware -> FATFS -> Mode: User-defined
 *   2. Connectivity -> SPI2 -> Full-Duplex Master
 *      - SCK:  PB13
 *      - MISO: PB14
 *      - MOSI: PB15
 *      - CS:   PB12 (GPIO_Output)
 *   3. Provide the FatFS <-> SPI glue in user_diskio.c (sector read/write
 *      via HAL_SPI_TransmitReceive, CS toggled on PB12).
 *   4. Regenerate code. ff.h will then be on the include path.
 *
 * Until FatFS is enabled, SD_Logger_Init() returns non-zero and all
 * _Append / _Sync calls no-op (guarded by sd_ready).
 */

#include "sd_logger.h"
#include <stdio.h>
#include <string.h>

#if __has_include("ff.h")
#include "ff.h"
#define SD_FATFS_AVAILABLE 1
#else
#define SD_FATFS_AVAILABLE 0
#endif

static uint8_t sd_ready = 0;

#if SD_FATFS_AVAILABLE
static FATFS fs;
static FIL   fil;
static const char *SD_PATH   = "0:/temp.csv";
static const char *SD_HEADER = "timestamp_ms,t_internal_c,t_external_c\r\n";
#endif

uint8_t SD_Logger_Init(void)
{
    sd_ready = 0;
#if SD_FATFS_AVAILABLE
    if (f_mount(&fs, "0:/", 1) != FR_OK) return 1;

    FILINFO fno;
    uint8_t need_header = (f_stat(SD_PATH, &fno) != FR_OK);

    if (f_open(&fil, SD_PATH, FA_OPEN_APPEND | FA_WRITE | FA_READ) != FR_OK) {
        return 2;
    }

    if (need_header) {
        UINT bw = 0;
        f_write(&fil, SD_HEADER, (UINT)strlen(SD_HEADER), &bw);
        f_sync(&fil);
    }

    sd_ready = 1;
    return 0;
#else
    return 0xFF;
#endif
}

void SD_Logger_Append(uint32_t ts_ms, float t_int, float t_ext)
{
    if (!sd_ready) return;
#if SD_FATFS_AVAILABLE
    char line[96];
    int n = snprintf(line, sizeof(line), "%lu,%.2f,%.2f\r\n",
                     (unsigned long)ts_ms, t_int, t_ext);
    if (n <= 0) return;
    UINT bw = 0;
    f_write(&fil, line, (UINT)n, &bw);
#else
    (void)ts_ms; (void)t_int; (void)t_ext;
#endif
}

void SD_Logger_Sync(void)
{
    if (!sd_ready) return;
#if SD_FATFS_AVAILABLE
    f_sync(&fil);
#endif
}

uint8_t SD_Logger_IsReady(void)
{
    return sd_ready;
}
