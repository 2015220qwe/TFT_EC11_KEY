/**
  ******************************************************************************
  * @file    bsp_key.c
  * @author  TFT_EC11_KEY Project
  * @brief   独立按键驱动实现 - STM32F407标准库版本
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_key.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    uint8_t  current_state;   /* 当前状态: 0-释放, 1-按下 */
    uint8_t  last_state;      /* 上次状态 */
    uint32_t press_time;      /* 按下时间戳(ms) */
    uint8_t  long_press_flag; /* 长按标志 */
} key_state_t;

/* Private variables ---------------------------------------------------------*/
static key_config_t key_configs[KEY_NUM_MAX];
static key_state_t  key_states[KEY_NUM_MAX];
static uint8_t      key_num = 0;
static key_event_callback_t event_callback = NULL;

/* 外部时间戳函数(需要用户实现) */
extern uint32_t bsp_ec11_get_tick(void);  /* 复用EC11的时间戳函数 */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  读取按键引脚状态
 * @param  key_id: 按键ID
 * @retval 按键状态: 0-未按下, 1-按下
 */
static uint8_t key_read_pin(key_id_t key_id)
{
    uint8_t pin_state;

    if (key_id >= key_num) {
        return 0;
    }

    pin_state = GPIO_ReadInputDataBit(key_configs[key_id].gpio_port,
                                      key_configs[key_id].gpio_pin);

    /* 根据有效电平返回逻辑状态 */
    if (key_configs[key_id].active_level == 0) {
        return (pin_state == 0) ? 1 : 0;  /* 低电平有效 */
    } else {
        return (pin_state == 1) ? 1 : 0;  /* 高电平有效 */
    }
}

/**
 * @brief  按键模块初始化
 * @param  config: 按键配置数组
 * @param  num: 按键数量
 * @retval None
 */
void bsp_key_init(const key_config_t *config, uint8_t num)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    uint8_t i;

    if (num > KEY_NUM_MAX) {
        num = KEY_NUM_MAX;
    }

    key_num = num;

    /* 保存配置 */
    for (i = 0; i < num; i++) {
        key_configs[i] = config[i];

        /* 初始化状态 */
        key_states[i].current_state = 0;
        key_states[i].last_state = 0;
        key_states[i].press_time = 0;
        key_states[i].long_press_flag = 0;
    }

    /* 配置GPIO */
    for (i = 0; i < num; i++) {
        /* 使能GPIO时钟 (这里简化处理，实际应根据端口判断) */
        if (key_configs[i].gpio_port == GPIOA) {
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        } else if (key_configs[i].gpio_port == GPIOB) {
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        } else if (key_configs[i].gpio_port == GPIOC) {
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        }

        /* 配置为输入模式 */
        GPIO_InitStructure.GPIO_Pin = key_configs[i].gpio_pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

        /* 根据有效电平配置上下拉 */
        if (key_configs[i].active_level == 0) {
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;  /* 低电平有效，上拉 */
        } else {
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  /* 高电平有效，下拉 */
        }

        GPIO_Init(key_configs[i].gpio_port, &GPIO_InitStructure);
    }
}

/**
 * @brief  按键扫描函数
 * @param  None
 * @retval None
 */
void bsp_key_scan(void)
{
    uint8_t i;
    uint32_t current_time = bsp_ec11_get_tick();

    for (i = 0; i < key_num; i++) {
        uint8_t key_pin = key_read_pin((key_id_t)i);

        /* 更新当前状态 */
        key_states[i].current_state = key_pin;

        /* 检测按键事件 */
        if (key_states[i].current_state == 1) {
            /* 按键按下 */
            if (key_states[i].last_state == 0) {
                /* 按键刚按下 */
                key_states[i].press_time = current_time;
                key_states[i].long_press_flag = 0;
            } else {
                /* 按键保持按下 - 检测长按 */
                uint32_t press_duration = current_time - key_states[i].press_time;

                if (press_duration >= KEY_LONG_PRESS_TIME_MS && !key_states[i].long_press_flag) {
                    /* 触发长按事件 */
                    key_states[i].long_press_flag = 1;

                    if (event_callback != NULL) {
                        event_callback((key_id_t)i, KEY_EVENT_LONG_PRESS);
                    }
                }
            }
        } else {
            /* 按键释放 */
            if (key_states[i].last_state == 1) {
                /* 按键刚释放 */
                uint32_t press_duration = current_time - key_states[i].press_time;

                if (!key_states[i].long_press_flag) {
                    /* 触发短按事件 */
                    if (event_callback != NULL) {
                        event_callback((key_id_t)i, KEY_EVENT_SHORT_PRESS);
                    }
                }

                /* 触发释放事件 */
                if (event_callback != NULL) {
                    event_callback((key_id_t)i, KEY_EVENT_RELEASE);
                }
            }
        }

        /* 更新上次状态 */
        key_states[i].last_state = key_states[i].current_state;
    }
}

/**
 * @brief  获取按键状态
 * @param  key_id: 按键ID
 * @retval 0-未按下, 1-按下
 */
uint8_t bsp_key_get_state(key_id_t key_id)
{
    if (key_id >= key_num) {
        return 0;
    }

    return key_states[key_id].current_state;
}

/**
 * @brief  注册按键事件回调函数
 * @param  callback: 回调函数指针
 * @retval None
 */
void bsp_key_register_callback(key_event_callback_t callback)
{
    event_callback = callback;
}
