/**
  ******************************************************************************
  * @file    menu_config.c
  * @author  TFT_EC11_KEY Project
  * @brief   菜单配置参数保存/加载实现
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "menu_config.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static menu_param_config_t param_configs[MENU_CONFIG_MAX_PARAMS];
static uint8_t param_count = 0;
static menu_config_data_t config_data = {0};
static uint8_t config_dirty = 0;          /* 配置脏标志 */
static uint32_t last_modify_time = 0;     /* 上次修改时间 */

/* 外部时间戳函数 */
extern uint32_t bsp_ec11_get_tick(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  计算CRC32校验和
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @retval CRC32值
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/**
 * @brief  验证配置数据有效性
 * @param  data: 配置数据指针
 * @retval 1-有效, 0-无效
 */
static uint8_t validate_config_data(const menu_config_data_t *data)
{
    uint32_t crc_calc;

    /* 检查魔术字 */
    if (data->magic != MENU_CONFIG_MAGIC) {
        return 0;
    }

    /* 检查版本号 */
    if (data->version != MENU_CONFIG_VERSION) {
        return 0;
    }

    /* 检查参数数量 */
    if (data->param_count > MENU_CONFIG_MAX_PARAMS) {
        return 0;
    }

    /* 计算并验证CRC */
    crc_calc = calculate_crc32((const uint8_t*)data,
                               sizeof(menu_config_data_t) - sizeof(uint32_t));

    if (crc_calc != data->crc32) {
        return 0;
    }

    return 1;
}

/**
 * @brief  查找参数配置
 * @param  name: 参数名称
 * @retval 参数索引, -1表示未找到
 */
static int find_param_index(const char *name)
{
    uint8_t i;

    for (i = 0; i < param_count; i++) {
        if (strcmp(param_configs[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  初始化菜单配置系统
 */
int menu_config_init(const menu_param_config_t *params, uint8_t count)
{
    uint8_t i;

    if (count > MENU_CONFIG_MAX_PARAMS) {
        return -1;
    }

    /* 保存参数配置 */
    param_count = count;
    for (i = 0; i < count; i++) {
        param_configs[i] = params[i];
    }

    /* 初始化配置数据结构 */
    memset(&config_data, 0, sizeof(config_data));
    config_data.magic = MENU_CONFIG_MAGIC;
    config_data.version = MENU_CONFIG_VERSION;
    config_data.param_count = count;

    /* 设置默认值 */
    for (i = 0; i < count; i++) {
        strncpy(config_data.params[i].name, param_configs[i].name, 15);
        config_data.params[i].name[15] = '\0';
        config_data.params[i].value = param_configs[i].default_val;
    }

    /* 尝试从EEPROM加载 */
    if (menu_config_load() != 0) {
        /* 加载失败，使用默认值 */
        menu_config_reset_to_default();
    }

    config_dirty = 0;

    return 0;
}

/**
 * @brief  从EEPROM加载配置
 */
int menu_config_load(void)
{
    menu_config_data_t temp_data;
    uint8_t i;
    int param_idx;

    /* 从EEPROM读取 */
    if (menu_config_eeprom_read(0, (uint8_t*)&temp_data, sizeof(temp_data)) != 0) {
        return -1;
    }

    /* 验证数据 */
    if (!validate_config_data(&temp_data)) {
        return -1;
    }

    /* 应用配置到内存 */
    for (i = 0; i < temp_data.param_count; i++) {
        param_idx = find_param_index(temp_data.params[i].name);

        if (param_idx >= 0) {
            /* 根据类型赋值 */
            switch (param_configs[param_idx].type) {
                case MENU_PARAM_TYPE_INT32:
                    *(int32_t*)param_configs[param_idx].ptr = temp_data.params[i].value;
                    break;

                case MENU_PARAM_TYPE_UINT8:
                    *(uint8_t*)param_configs[param_idx].ptr = (uint8_t)temp_data.params[i].value;
                    break;

                case MENU_PARAM_TYPE_FLOAT:
                    /* 浮点数暂不支持 */
                    break;
            }
        }
    }

    /* 保存到本地副本 */
    config_data = temp_data;

    return 0;
}

/**
 * @brief  保存配置到EEPROM
 */
int menu_config_save(uint8_t immediate)
{
    uint8_t i;

    if (!immediate) {
        /* 延迟保存 */
        menu_config_mark_dirty();
        return 0;
    }

    /* 从内存中读取当前值 */
    for (i = 0; i < param_count; i++) {
        switch (param_configs[i].type) {
            case MENU_PARAM_TYPE_INT32:
                config_data.params[i].value = *(int32_t*)param_configs[i].ptr;
                break;

            case MENU_PARAM_TYPE_UINT8:
                config_data.params[i].value = *(uint8_t*)param_configs[i].ptr;
                break;

            case MENU_PARAM_TYPE_FLOAT:
                /* 浮点数暂不支持 */
                break;
        }
    }

    /* 计算CRC */
    config_data.crc32 = calculate_crc32((const uint8_t*)&config_data,
                                        sizeof(menu_config_data_t) - sizeof(uint32_t));

    /* 写入EEPROM */
    if (menu_config_eeprom_write(0, (const uint8_t*)&config_data, sizeof(config_data)) != 0) {
        return -1;
    }

    config_dirty = 0;

    return 0;
}

/**
 * @brief  恢复出厂设置
 */
void menu_config_reset_to_default(void)
{
    uint8_t i;

    /* 应用默认值 */
    for (i = 0; i < param_count; i++) {
        switch (param_configs[i].type) {
            case MENU_PARAM_TYPE_INT32:
                *(int32_t*)param_configs[i].ptr = param_configs[i].default_val;
                break;

            case MENU_PARAM_TYPE_UINT8:
                *(uint8_t*)param_configs[i].ptr = (uint8_t)param_configs[i].default_val;
                break;

            case MENU_PARAM_TYPE_FLOAT:
                /* 浮点数暂不支持 */
                break;
        }

        config_data.params[i].value = param_configs[i].default_val;
    }

    /* 立即保存 */
    menu_config_save(1);
}

/**
 * @brief  标记参数已修改
 */
void menu_config_mark_dirty(void)
{
    config_dirty = 1;
    last_modify_time = bsp_ec11_get_tick();
}

/**
 * @brief  周期性任务
 */
void menu_config_task(void)
{
    uint32_t current_time = bsp_ec11_get_tick();

    /* 检查是否需要延迟保存 */
    if (config_dirty) {
        if ((current_time - last_modify_time) >= MENU_CONFIG_SAVE_DELAY_MS) {
            /* 触发保存 */
            menu_config_save(1);
        }
    }
}

/**
 * @brief  获取配置数据指针
 */
menu_config_data_t* menu_config_get_data(void)
{
    return &config_data;
}

/* 弱定义的EEPROM接口(用户需要实现) ----------------------------------------*/

__weak int menu_config_eeprom_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    /* 默认实现：从STM32内部Flash模拟EEPROM */
    /* 用户需要根据实际硬件实现此函数 */
    return -1;  /* 返回失败 */
}

__weak int menu_config_eeprom_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    /* 默认实现：写入STM32内部Flash模拟EEPROM */
    /* 用户需要根据实际硬件实现此函数 */
    return -1;  /* 返回失败 */
}
