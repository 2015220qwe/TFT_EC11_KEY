/**
 * @file diskio.h
 * @brief FatFS底层磁盘IO接口定义
 * @note 基于FatFs R0.15
 * @date 2025-12-12
 */

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*=============================================================================
 *                              状态类型定义
 *============================================================================*/

/* 磁盘状态 */
typedef uint8_t DSTATUS;

/* 磁盘操作结果 */
typedef enum {
    RES_OK = 0,     /* 成功 */
    RES_ERROR,      /* 读写错误 */
    RES_WRPRT,      /* 写保护 */
    RES_NOTRDY,     /* 设备未就绪 */
    RES_PARERR      /* 参数错误 */
} DRESULT;

/*=============================================================================
 *                              磁盘状态位定义
 *============================================================================*/

#define STA_NOINIT      0x01    /* 驱动器未初始化 */
#define STA_NODISK      0x02    /* 无介质 */
#define STA_PROTECT     0x04    /* 写保护 */

/*=============================================================================
 *                              通用命令定义
 *============================================================================*/

/* 通用命令 (用于FatFs) */
#define CTRL_SYNC           0   /* 完成挂起的写入 */
#define GET_SECTOR_COUNT    1   /* 获取扇区数量 */
#define GET_SECTOR_SIZE     2   /* 获取扇区大小 */
#define GET_BLOCK_SIZE      3   /* 获取擦除块大小 */
#define CTRL_TRIM           4   /* 通知可擦除块 */

/* 通用命令 (非FatFs必需) */
#define CTRL_POWER          5   /* 获取/设置电源状态 */
#define CTRL_LOCK           6   /* 锁定/解锁介质移除 */
#define CTRL_EJECT          7   /* 弹出介质 */
#define CTRL_FORMAT         8   /* 在物理驱动器上创建文件系统 */

/* MMC/SDC特定命令 */
#define MMC_GET_TYPE        10  /* 获取卡类型 */
#define MMC_GET_CSD         11  /* 获取CSD */
#define MMC_GET_CID         12  /* 获取CID */
#define MMC_GET_OCR         13  /* 获取OCR */
#define MMC_GET_SDSTAT      14  /* 获取SD状态 */
#define ISDIO_READ          55  /* 从SD iSDIO读取 */
#define ISDIO_WRITE         56  /* 写入SD iSDIO */
#define ISDIO_MRITE         57  /* 屏蔽写入SD iSDIO */

/* ATA/CF特定命令 */
#define ATA_GET_REV         20  /* 获取固件版本 */
#define ATA_GET_MODEL       21  /* 获取型号名称 */
#define ATA_GET_SN          22  /* 获取序列号 */

/*=============================================================================
 *                              驱动器号定义
 *============================================================================*/

#define DEV_SD      0   /* SD卡 */
#define DEV_MMC     1   /* MMC卡 */
#define DEV_USB     2   /* USB存储 */

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief 初始化磁盘驱动器
 * @param pdrv 物理驱动器号
 * @retval 磁盘状态
 */
DSTATUS disk_initialize(uint8_t pdrv);

/**
 * @brief 获取磁盘状态
 * @param pdrv 物理驱动器号
 * @retval 磁盘状态
 */
DSTATUS disk_status(uint8_t pdrv);

/**
 * @brief 读取扇区
 * @param pdrv 物理驱动器号
 * @param buff 数据缓冲区
 * @param sector 扇区地址
 * @param count 扇区数量
 * @retval 操作结果
 */
DRESULT disk_read(uint8_t pdrv, uint8_t *buff, uint32_t sector, uint32_t count);

/**
 * @brief 写入扇区
 * @param pdrv 物理驱动器号
 * @param buff 数据缓冲区
 * @param sector 扇区地址
 * @param count 扇区数量
 * @retval 操作结果
 */
DRESULT disk_write(uint8_t pdrv, const uint8_t *buff, uint32_t sector, uint32_t count);

/**
 * @brief IO控制
 * @param pdrv 物理驱动器号
 * @param cmd 控制命令
 * @param buff 缓冲区
 * @retval 操作结果
 */
DRESULT disk_ioctl(uint8_t pdrv, uint8_t cmd, void *buff);

/**
 * @brief 获取FAT时间
 * @retval FAT格式的时间戳
 */
uint32_t get_fattime(void);

#ifdef __cplusplus
}
#endif

#endif /* _DISKIO_DEFINED */
