/**
 * @file diskio.c
 * @brief FatFS底层磁盘IO实现 - 对接SD卡驱动
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "diskio.h"
#include "bsp/bsp_sdcard.h"

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static volatile DSTATUS Stat = STA_NOINIT;

/*=============================================================================
 *                              函数实现
 *============================================================================*/

/**
 * @brief 初始化磁盘驱动器
 */
DSTATUS disk_initialize(uint8_t pdrv)
{
    sd_result_t res;

    switch (pdrv) {
    case DEV_SD:
        res = bsp_sd_init();
        if (res == SD_OK) {
            Stat &= ~STA_NOINIT;
        } else {
            Stat = STA_NOINIT;
        }
        return Stat;

    default:
        break;
    }

    return STA_NOINIT;
}

/**
 * @brief 获取磁盘状态
 */
DSTATUS disk_status(uint8_t pdrv)
{
    switch (pdrv) {
    case DEV_SD:
        if (bsp_sd_is_ready()) {
            Stat &= ~STA_NOINIT;
        } else {
            Stat |= STA_NOINIT;
        }
        return Stat;

    default:
        break;
    }

    return STA_NOINIT;
}

/**
 * @brief 读取扇区
 */
DRESULT disk_read(uint8_t pdrv, uint8_t *buff, uint32_t sector, uint32_t count)
{
    sd_result_t res;

    if (pdrv != DEV_SD || !count) {
        return RES_PARERR;
    }

    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    if (count == 1) {
        res = bsp_sd_read_sector(sector, buff);
    } else {
        res = bsp_sd_read_sectors(sector, buff, count);
    }

    if (res == SD_OK) {
        return RES_OK;
    }

    return RES_ERROR;
}

/**
 * @brief 写入扇区
 */
DRESULT disk_write(uint8_t pdrv, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    sd_result_t res;

    if (pdrv != DEV_SD || !count) {
        return RES_PARERR;
    }

    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    if (Stat & STA_PROTECT) {
        return RES_WRPRT;
    }

    if (count == 1) {
        res = bsp_sd_write_sector(sector, buff);
    } else {
        res = bsp_sd_write_sectors(sector, buff, count);
    }

    if (res == SD_OK) {
        return RES_OK;
    }

    return RES_ERROR;
}

/**
 * @brief IO控制
 */
DRESULT disk_ioctl(uint8_t pdrv, uint8_t cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    if (pdrv != DEV_SD) {
        return RES_PARERR;
    }

    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    switch (cmd) {
    case CTRL_SYNC:
        /* 确保写入完成 */
        if (bsp_sd_sync() == SD_OK) {
            res = RES_OK;
        }
        break;

    case GET_SECTOR_COUNT:
        /* 获取扇区总数 */
        *(uint32_t *)buff = bsp_sd_get_sector_count();
        res = RES_OK;
        break;

    case GET_SECTOR_SIZE:
        /* 获取扇区大小 */
        *(uint16_t *)buff = bsp_sd_get_sector_size();
        res = RES_OK;
        break;

    case GET_BLOCK_SIZE:
        /* 获取擦除块大小 (扇区单位) */
        *(uint32_t *)buff = bsp_sd_get_block_size();
        res = RES_OK;
        break;

    case CTRL_TRIM:
        /* 可选: 通知可擦除扇区 */
        res = RES_OK;
        break;

    default:
        res = RES_PARERR;
        break;
    }

    return res;
}

/**
 * @brief 获取FAT时间
 * @note 格式: bit31:25=年(0=1980), bit24:21=月, bit20:16=日
 *             bit15:11=时, bit10:5=分, bit4:0=秒/2
 */
uint32_t get_fattime(void)
{
    /* 返回固定时间: 2025-12-12 12:00:00 */
    return ((uint32_t)(2025 - 1980) << 25)  /* 年 */
         | ((uint32_t)12 << 21)              /* 月 */
         | ((uint32_t)12 << 16)              /* 日 */
         | ((uint32_t)12 << 11)              /* 时 */
         | ((uint32_t)0 << 5)                /* 分 */
         | ((uint32_t)0 >> 1);               /* 秒 */

    /* 如果有RTC，可以这样实现:
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    return ((uint32_t)(date.Year + 20) << 25)
         | ((uint32_t)date.Month << 21)
         | ((uint32_t)date.Date << 16)
         | ((uint32_t)time.Hours << 11)
         | ((uint32_t)time.Minutes << 5)
         | ((uint32_t)time.Seconds >> 1);
    */
}
