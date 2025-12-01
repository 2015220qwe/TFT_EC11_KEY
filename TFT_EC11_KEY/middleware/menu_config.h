/**
  ******************************************************************************
  * @file    menu_config.h
  * @author  TFT_EC11_KEY Project
  * @brief   菜单配置参数保存/加载模块 - 支持EEPROM持久化
  ******************************************************************************
  * @attention
  *
  * 功能特性:
  * - 自动保存菜单配置到EEPROM/Flash
  * - 支持数值型和开关型参数
  * - 自动CRC校验，防止数据损坏
  * - 延迟写入机制，减少Flash擦写次数
  *
  ******************************************************************************
  */

#ifndef __MENU_CONFIG_H
#define __MENU_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "menu_core.h"

/* 配置选项 ------------------------------------------------------------------*/
#define MENU_CONFIG_MAX_PARAMS      16    /* 最多支持的配置参数数量 */
#define MENU_CONFIG_MAGIC           0x4D434647  /* "MCFG" 魔术字 */
#define MENU_CONFIG_VERSION         0x0100      /* 版本号 v1.0 */
#define MENU_CONFIG_SAVE_DELAY_MS   3000  /* 延迟保存时间(毫秒) */

/* 参数类型 ------------------------------------------------------------------*/
typedef enum {
    MENU_PARAM_TYPE_INT32 = 0,   /* 32位整数 */
    MENU_PARAM_TYPE_UINT8,       /* 8位无符号整数(用于开关) */
    MENU_PARAM_TYPE_FLOAT        /* 浮点数(预留) */
} menu_param_type_t;

/* 参数配置结构体 ------------------------------------------------------------*/
typedef struct {
    char              name[16];    /* 参数名称 */
    menu_param_type_t type;        /* 参数类型 */
    void             *ptr;         /* 参数指针 */
    int32_t           default_val; /* 默认值 */
} menu_param_config_t;

/* 存储在EEPROM中的数据结构 ------------------------------------------------*/
typedef struct {
    uint32_t magic;                /* 魔术字，用于验证数据有效性 */
    uint16_t version;              /* 配置版本号 */
    uint16_t param_count;          /* 参数数量 */

    /* 参数数据 */
    struct {
        char    name[16];
        int32_t value;
    } params[MENU_CONFIG_MAX_PARAMS];

    uint32_t crc32;               /* CRC32校验和 */
} menu_config_data_t;

/* API函数声明 ---------------------------------------------------------------*/

/**
 * @brief  初始化菜单配置系统
 * @param  params: 参数配置数组
 * @param  count: 参数数量
 * @retval 0-成功, -1-失败
 */
int menu_config_init(const menu_param_config_t *params, uint8_t count);

/**
 * @brief  从EEPROM加载配置
 * @param  None
 * @retval 0-成功, -1-失败(使用默认值)
 */
int menu_config_load(void);

/**
 * @brief  保存配置到EEPROM
 * @param  immediate: 1-立即保存, 0-延迟保存
 * @retval 0-成功, -1-失败
 */
int menu_config_save(uint8_t immediate);

/**
 * @brief  恢复出厂设置
 * @param  None
 * @retval None
 */
void menu_config_reset_to_default(void);

/**
 * @brief  标记参数已修改(触发延迟保存)
 * @param  None
 * @retval None
 */
void menu_config_mark_dirty(void);

/**
 * @brief  周期性任务(需要在主循环中调用)
 * @param  None
 * @retval None
 * @note   处理延迟保存逻辑
 */
void menu_config_task(void);

/**
 * @brief  获取配置数据指针(用于调试)
 * @param  None
 * @retval 配置数据指针
 */
menu_config_data_t* menu_config_get_data(void);

/* 用户需要实现的底层接口 ---------------------------------------------------*/

/**
 * @brief  从EEPROM读取数据
 * @param  addr: 地址偏移
 * @param  buf: 数据缓冲区
 * @param  len: 数据长度
 * @retval 0-成功, -1-失败
 */
int menu_config_eeprom_read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * @brief  向EEPROM写入数据
 * @param  addr: 地址偏移
 * @param  buf: 数据缓冲区
 * @param  len: 数据长度
 * @retval 0-成功, -1-失败
 */
int menu_config_eeprom_write(uint32_t addr, const uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MENU_CONFIG_H */
