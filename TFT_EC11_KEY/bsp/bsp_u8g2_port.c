/**
  ******************************************************************************
  * @file    bsp_u8g2_port.c
  * @author  TFT_EC11_KEY Project
  * @brief   U8G2显示库移植实现 - STM32F407标准库版本
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_u8g2_port.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"

/* 外部延时函数 */
extern void delay_us(uint32_t us);
extern void delay_ms(uint32_t ms);

/* Private functions ---------------------------------------------------------*/

#if U8G2_USE_HW_I2C
/**
 * @brief  硬件I2C初始化
 */
static void hw_i2c_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    /* 使能时钟 */
    RCC_AHB1PeriphClockCmd(U8G2_I2C_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(U8G2_I2C_PERIPH_CLK, ENABLE);

    /* 配置GPIO */
    GPIO_InitStructure.GPIO_Pin = U8G2_I2C_SCL_PIN | U8G2_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(U8G2_I2C_SCL_PORT, &GPIO_InitStructure);

    /* 配置复用功能 */
    GPIO_PinAFConfig(U8G2_I2C_SCL_PORT, GPIO_PinSource6, GPIO_AF_I2C1);
    GPIO_PinAFConfig(U8G2_I2C_SDA_PORT, GPIO_PinSource7, GPIO_AF_I2C1);

    /* 配置I2C */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 400000;  /* 400kHz */

    I2C_Init(U8G2_I2C_PORT, &I2C_InitStructure);
    I2C_Cmd(U8G2_I2C_PORT, ENABLE);
}
#endif

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  U8G2硬件初始化
 */
void bsp_u8g2_hw_init(u8g2_t *u8g2)
{
#if U8G2_USE_HW_I2C
    hw_i2c_init();

    /* 初始化U8G2 - SSD1306 OLED 128x64 I2C */
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0,
                                            u8g2_byte_hw_i2c,
                                            u8g2_gpio_and_delay_stm32);
#endif

    /* 初始化显示器 */
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_ClearBuffer(u8g2);
}

/**
 * @brief  GPIO和延时回调
 */
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            /* 已在hw_init中完成 */
            break;

        case U8X8_MSG_DELAY_NANO:
            /* 纳秒级延时(通过NOP指令) */
            __NOP();
            break;

        case U8X8_MSG_DELAY_100NANO:
            /* 100纳秒延时 */
            __NOP();
            break;

        case U8X8_MSG_DELAY_10MICRO:
            /* 10微秒延时 */
            delay_us(10);
            break;

        case U8X8_MSG_DELAY_MILLI:
            /* 毫秒延时 */
            delay_ms(arg_int);
            break;

        default:
            return 0;
    }

    return 1;
}

#if U8G2_USE_HW_I2C
/**
 * @brief  硬件I2C字节传输
 */
uint8_t u8g2_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    uint8_t *data;
    uint8_t i;

    switch (msg) {
        case U8X8_MSG_BYTE_INIT:
            /* 已在hw_init中完成 */
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            /* 开始传输 */
            I2C_GenerateSTART(U8G2_I2C_PORT, ENABLE);
            while (!I2C_CheckEvent(U8G2_I2C_PORT, I2C_EVENT_MASTER_MODE_SELECT));

            /* 发送从设备地址 */
            I2C_Send7bitAddress(U8G2_I2C_PORT, u8x8_GetI2CAddress(u8x8), I2C_Direction_Transmitter);
            while (!I2C_CheckEvent(U8G2_I2C_PORT, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
            break;

        case U8X8_MSG_BYTE_SEND:
            /* 发送数据 */
            data = (uint8_t *)arg_ptr;
            for (i = 0; i < arg_int; i++) {
                I2C_SendData(U8G2_I2C_PORT, data[i]);
                while (!I2C_CheckEvent(U8G2_I2C_PORT, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
            }
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            /* 结束传输 */
            I2C_GenerateSTOP(U8G2_I2C_PORT, ENABLE);
            break;

        default:
            return 0;
    }

    return 1;
}
#endif
