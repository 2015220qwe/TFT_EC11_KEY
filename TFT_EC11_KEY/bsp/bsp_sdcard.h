/**
 * @file bsp_sdcard.h
 * @brief SD卡SPI驱动 - 支持SDSC/SDHC/SDXC
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 *
 * @note 硬件平台: STM32F407VGT6
 * @note 通信接口: SPI
 * @note 支持FatFS文件系统
 */

#ifndef __BSP_SDCARD_H
#define __BSP_SDCARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/*=============================================================================
 *                              硬件配置
 *============================================================================*/

/* SPI配置 */
#define SD_SPI              SPI2
#define SD_SPI_CLK          RCC_APB1Periph_SPI2

/* GPIO配置 */
#define SD_GPIO_PORT        GPIOB
#define SD_GPIO_CLK         RCC_AHB1Periph_GPIOB

#define SD_SCK_PIN          GPIO_Pin_13
#define SD_SCK_PINSOURCE    GPIO_PinSource13
#define SD_MISO_PIN         GPIO_Pin_14
#define SD_MISO_PINSOURCE   GPIO_PinSource14
#define SD_MOSI_PIN         GPIO_Pin_15
#define SD_MOSI_PINSOURCE   GPIO_PinSource15
#define SD_CS_PIN           GPIO_Pin_12

/* CS控制宏 */
#define SD_CS_LOW()         GPIO_ResetBits(SD_GPIO_PORT, SD_CS_PIN)
#define SD_CS_HIGH()        GPIO_SetBits(SD_GPIO_PORT, SD_CS_PIN)

/*=============================================================================
 *                              SD卡命令定义
 *============================================================================*/

#define SD_CMD0     0       /* GO_IDLE_STATE */
#define SD_CMD1     1       /* SEND_OP_COND */
#define SD_CMD8     8       /* SEND_IF_COND */
#define SD_CMD9     9       /* SEND_CSD */
#define SD_CMD10    10      /* SEND_CID */
#define SD_CMD12    12      /* STOP_TRANSMISSION */
#define SD_CMD16    16      /* SET_BLOCKLEN */
#define SD_CMD17    17      /* READ_SINGLE_BLOCK */
#define SD_CMD18    18      /* READ_MULTIPLE_BLOCK */
#define SD_CMD23    23      /* SET_BLOCK_COUNT */
#define SD_CMD24    24      /* WRITE_BLOCK */
#define SD_CMD25    25      /* WRITE_MULTIPLE_BLOCK */
#define SD_CMD55    55      /* APP_CMD */
#define SD_CMD58    58      /* READ_OCR */
#define SD_ACMD41   41      /* SD_SEND_OP_COND */

/*=============================================================================
 *                              SD卡响应定义
 *============================================================================*/

#define SD_R1_IDLE_STATE        0x01
#define SD_R1_ERASE_RESET       0x02
#define SD_R1_ILLEGAL_CMD       0x04
#define SD_R1_CRC_ERROR         0x08
#define SD_R1_ERASE_SEQ_ERROR   0x10
#define SD_R1_ADDRESS_ERROR     0x20
#define SD_R1_PARAM_ERROR       0x40

/* 数据令牌 */
#define SD_DATA_TOKEN_CMD17     0xFE
#define SD_DATA_TOKEN_CMD18     0xFE
#define SD_DATA_TOKEN_CMD24     0xFE
#define SD_DATA_TOKEN_CMD25     0xFC
#define SD_STOP_TOKEN_CMD25     0xFD

/*=============================================================================
 *                              SD卡类型定义
 *============================================================================*/

typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_MMC,            /* MMC卡 */
    SD_TYPE_SDSC_V1,        /* SD V1.x (标准容量) */
    SD_TYPE_SDSC_V2,        /* SD V2.0 SDSC (标准容量) */
    SD_TYPE_SDHC_SDXC       /* SDHC/SDXC (高容量/扩展容量) */
} sd_type_t;

/*=============================================================================
 *                              SD卡信息结构
 *============================================================================*/

typedef struct {
    sd_type_t type;         /* 卡类型 */
    uint32_t capacity;      /* 容量 (扇区数) */
    uint32_t block_size;    /* 块大小 (字节) */
} sd_info_t;

/*=============================================================================
 *                              错误码定义
 *============================================================================*/

typedef enum {
    SD_OK = 0,
    SD_ERROR,
    SD_ERROR_TIMEOUT,
    SD_ERROR_PARAM,
    SD_ERROR_NO_CARD,
    SD_ERROR_UNSUPPORTED,
    SD_ERROR_CRC,
    SD_ERROR_WRITE_PROTECT,
    SD_ERROR_READ,
    SD_ERROR_WRITE
} sd_result_t;

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/**
 * @brief SD卡初始化
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_init(void);

/**
 * @brief SD卡反初始化
 */
void bsp_sd_deinit(void);

/**
 * @brief 读取单个扇区
 * @param sector 扇区号
 * @param buf 数据缓冲区 (512字节)
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_read_sector(uint32_t sector, uint8_t *buf);

/**
 * @brief 读取多个扇区
 * @param sector 起始扇区号
 * @param buf 数据缓冲区
 * @param count 扇区数量
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_read_sectors(uint32_t sector, uint8_t *buf, uint32_t count);

/**
 * @brief 写入单个扇区
 * @param sector 扇区号
 * @param buf 数据缓冲区 (512字节)
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_write_sector(uint32_t sector, const uint8_t *buf);

/**
 * @brief 写入多个扇区
 * @param sector 起始扇区号
 * @param buf 数据缓冲区
 * @param count 扇区数量
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_write_sectors(uint32_t sector, const uint8_t *buf, uint32_t count);

/**
 * @brief 获取SD卡信息
 * @param info 信息结构体指针
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_get_info(sd_info_t *info);

/**
 * @brief 获取SD卡扇区数
 * @retval 扇区数 (0表示失败)
 */
uint32_t bsp_sd_get_sector_count(void);

/**
 * @brief 获取SD卡扇区大小
 * @retval 扇区大小 (字节)
 */
uint16_t bsp_sd_get_sector_size(void);

/**
 * @brief 获取SD卡块大小
 * @retval 块大小 (扇区数)
 */
uint32_t bsp_sd_get_block_size(void);

/**
 * @brief 检测SD卡是否就绪
 * @retval 1:就绪 0:未就绪
 */
uint8_t bsp_sd_is_ready(void);

/**
 * @brief 同步 (确保写入完成)
 * @retval SD_OK:成功 其他:错误码
 */
sd_result_t bsp_sd_sync(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_SDCARD_H */
