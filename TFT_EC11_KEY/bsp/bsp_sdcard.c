/**
 * @file bsp_sdcard.c
 * @brief SD卡SPI驱动实现
 * @author Claude Code
 * @version 1.0.0
 * @date 2025-12-12
 */

#include "bsp_sdcard.h"

/*=============================================================================
 *                              私有变量
 *============================================================================*/

static sd_type_t sd_card_type = SD_TYPE_UNKNOWN;
static uint8_t sd_initialized = 0;

/* 延时函数 (需外部实现) */
extern void delay_ms(uint32_t ms);

/*=============================================================================
 *                              私有函数声明
 *============================================================================*/

static void sd_gpio_init(void);
static void sd_spi_init(void);
static void sd_spi_set_speed(uint8_t high_speed);
static uint8_t sd_spi_rw_byte(uint8_t data);
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg);
static uint8_t sd_wait_ready(uint16_t timeout_ms);
static int sd_receive_data(uint8_t *buf, uint16_t len);
static int sd_send_data(const uint8_t *buf, uint8_t token);

/*=============================================================================
 *                              SPI底层函数
 *============================================================================*/

/**
 * @brief GPIO初始化
 */
static void sd_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIO时钟 */
    RCC_AHB1PeriphClockCmd(SD_GPIO_CLK, ENABLE);

    /* 配置CS引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = SD_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SD_GPIO_PORT, &GPIO_InitStructure);

    /* CS默认高 */
    SD_CS_HIGH();
}

/**
 * @brief SPI初始化
 */
static void sd_spi_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    /* 使能时钟 */
    RCC_APB1PeriphClockCmd(SD_SPI_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(SD_GPIO_CLK, ENABLE);

    /* 配置SPI引脚为复用功能 */
    GPIO_InitStructure.GPIO_Pin = SD_SCK_PIN | SD_MISO_PIN | SD_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SD_GPIO_PORT, &GPIO_InitStructure);

    /* 配置复用功能 */
    GPIO_PinAFConfig(SD_GPIO_PORT, SD_SCK_PINSOURCE, GPIO_AF_SPI2);
    GPIO_PinAFConfig(SD_GPIO_PORT, SD_MISO_PINSOURCE, GPIO_AF_SPI2);
    GPIO_PinAFConfig(SD_GPIO_PORT, SD_MOSI_PINSOURCE, GPIO_AF_SPI2);

    /* 配置SPI - 初始化时使用低速 */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;  /* 低速初始化 */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SD_SPI, &SPI_InitStructure);

    SPI_Cmd(SD_SPI, ENABLE);
}

/**
 * @brief 设置SPI速度
 * @param high_speed 1:高速 0:低速
 */
static void sd_spi_set_speed(uint8_t high_speed)
{
    SPI_Cmd(SD_SPI, DISABLE);

    if (high_speed) {
        SD_SPI->CR1 &= ~SPI_BaudRatePrescaler_256;
        SD_SPI->CR1 |= SPI_BaudRatePrescaler_4;  /* 高速 ~10.5MHz */
    } else {
        SD_SPI->CR1 &= ~SPI_BaudRatePrescaler_256;
        SD_SPI->CR1 |= SPI_BaudRatePrescaler_256;  /* 低速 ~164KHz */
    }

    SPI_Cmd(SD_SPI, ENABLE);
}

/**
 * @brief SPI读写一个字节
 */
static uint8_t sd_spi_rw_byte(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SD_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SD_SPI, data);
    while (SPI_I2S_GetFlagStatus(SD_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SD_SPI);
}

/**
 * @brief 等待SD卡就绪
 */
static uint8_t sd_wait_ready(uint16_t timeout_ms)
{
    uint8_t res;
    uint32_t timeout = timeout_ms * 10;  /* 简单计时 */

    do {
        res = sd_spi_rw_byte(0xFF);
        if (res == 0xFF) {
            return 1;  /* 就绪 */
        }
        timeout--;
    } while (timeout);

    return 0;  /* 超时 */
}

/**
 * @brief 发送SD命令
 * @param cmd 命令
 * @param arg 参数
 * @retval R1响应
 */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t res;
    uint8_t retry = 0;
    uint8_t crc = 0xFF;

    /* 等待SD卡就绪 */
    if (cmd != SD_CMD0) {
        if (!sd_wait_ready(500)) {
            return 0xFF;
        }
    }

    /* 发送命令 */
    sd_spi_rw_byte(0x40 | cmd);
    sd_spi_rw_byte((uint8_t)(arg >> 24));
    sd_spi_rw_byte((uint8_t)(arg >> 16));
    sd_spi_rw_byte((uint8_t)(arg >> 8));
    sd_spi_rw_byte((uint8_t)(arg));

    /* CRC */
    if (cmd == SD_CMD0) {
        crc = 0x95;  /* CMD0 CRC */
    } else if (cmd == SD_CMD8) {
        crc = 0x87;  /* CMD8 CRC */
    }
    sd_spi_rw_byte(crc);

    /* 跳过停止读取命令的额外字节 */
    if (cmd == SD_CMD12) {
        sd_spi_rw_byte(0xFF);
    }

    /* 等待响应 */
    retry = 200;
    do {
        res = sd_spi_rw_byte(0xFF);
    } while ((res & 0x80) && --retry);

    return res;
}

/**
 * @brief 接收数据块
 */
static int sd_receive_data(uint8_t *buf, uint16_t len)
{
    uint8_t token;
    uint32_t timeout = 100000;

    /* 等待数据令牌 */
    do {
        token = sd_spi_rw_byte(0xFF);
    } while ((token == 0xFF) && --timeout);

    if (token != SD_DATA_TOKEN_CMD17) {
        return -1;
    }

    /* 接收数据 */
    while (len--) {
        *buf++ = sd_spi_rw_byte(0xFF);
    }

    /* 丢弃CRC */
    sd_spi_rw_byte(0xFF);
    sd_spi_rw_byte(0xFF);

    return 0;
}

/**
 * @brief 发送数据块
 */
static int sd_send_data(const uint8_t *buf, uint8_t token)
{
    uint8_t res;

    /* 等待就绪 */
    if (!sd_wait_ready(500)) {
        return -1;
    }

    /* 发送令牌 */
    sd_spi_rw_byte(token);

    if (token != SD_STOP_TOKEN_CMD25) {
        /* 发送数据 */
        uint16_t i;
        for (i = 0; i < 512; i++) {
            sd_spi_rw_byte(buf[i]);
        }

        /* 发送CRC (dummy) */
        sd_spi_rw_byte(0xFF);
        sd_spi_rw_byte(0xFF);

        /* 接收响应 */
        res = sd_spi_rw_byte(0xFF);
        if ((res & 0x1F) != 0x05) {
            return -1;
        }
    }

    return 0;
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

/**
 * @brief SD卡初始化
 */
sd_result_t bsp_sd_init(void)
{
    uint8_t res;
    uint8_t ocr[4];
    uint16_t retry;
    uint8_t i;

    sd_card_type = SD_TYPE_UNKNOWN;
    sd_initialized = 0;

    /* 初始化GPIO和SPI */
    sd_gpio_init();
    sd_spi_init();

    /* 发送至少74个时钟脉冲，CS保持高 */
    SD_CS_HIGH();
    for (i = 0; i < 10; i++) {
        sd_spi_rw_byte(0xFF);
    }

    /* 进入SPI模式: CMD0 */
    SD_CS_LOW();
    retry = 20;
    do {
        res = sd_send_cmd(SD_CMD0, 0);
    } while ((res != SD_R1_IDLE_STATE) && --retry);
    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    if (res != SD_R1_IDLE_STATE) {
        return SD_ERROR_NO_CARD;
    }

    /* 检测卡版本: CMD8 */
    SD_CS_LOW();
    res = sd_send_cmd(SD_CMD8, 0x1AA);

    if (res == SD_R1_IDLE_STATE) {
        /* SD V2.0 */
        for (i = 0; i < 4; i++) {
            ocr[i] = sd_spi_rw_byte(0xFF);
        }
        SD_CS_HIGH();
        sd_spi_rw_byte(0xFF);

        if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
            /* 发送ACMD41初始化 */
            retry = 1000;
            do {
                SD_CS_LOW();
                sd_send_cmd(SD_CMD55, 0);
                SD_CS_HIGH();
                sd_spi_rw_byte(0xFF);

                SD_CS_LOW();
                res = sd_send_cmd(SD_ACMD41, 0x40000000);
                SD_CS_HIGH();
                sd_spi_rw_byte(0xFF);
            } while (res && --retry);

            if (retry == 0) {
                return SD_ERROR_TIMEOUT;
            }

            /* 读取OCR: CMD58 */
            SD_CS_LOW();
            res = sd_send_cmd(SD_CMD58, 0);
            if (res == 0) {
                for (i = 0; i < 4; i++) {
                    ocr[i] = sd_spi_rw_byte(0xFF);
                }
                /* 检查CCS位 (bit 30) */
                if (ocr[0] & 0x40) {
                    sd_card_type = SD_TYPE_SDHC_SDXC;
                } else {
                    sd_card_type = SD_TYPE_SDSC_V2;
                }
            }
            SD_CS_HIGH();
            sd_spi_rw_byte(0xFF);
        }
    } else {
        /* SD V1.x 或 MMC */
        SD_CS_HIGH();
        sd_spi_rw_byte(0xFF);

        SD_CS_LOW();
        sd_send_cmd(SD_CMD55, 0);
        SD_CS_HIGH();
        sd_spi_rw_byte(0xFF);

        SD_CS_LOW();
        res = sd_send_cmd(SD_ACMD41, 0);
        SD_CS_HIGH();
        sd_spi_rw_byte(0xFF);

        if (res <= 1) {
            /* SD V1.x */
            sd_card_type = SD_TYPE_SDSC_V1;
            retry = 1000;
            do {
                SD_CS_LOW();
                sd_send_cmd(SD_CMD55, 0);
                SD_CS_HIGH();
                sd_spi_rw_byte(0xFF);

                SD_CS_LOW();
                res = sd_send_cmd(SD_ACMD41, 0);
                SD_CS_HIGH();
                sd_spi_rw_byte(0xFF);
            } while (res && --retry);
        } else {
            /* MMC */
            sd_card_type = SD_TYPE_MMC;
            retry = 1000;
            do {
                SD_CS_LOW();
                res = sd_send_cmd(SD_CMD1, 0);
                SD_CS_HIGH();
                sd_spi_rw_byte(0xFF);
            } while (res && --retry);
        }

        if (retry == 0) {
            return SD_ERROR_TIMEOUT;
        }

        /* 设置块大小为512 */
        SD_CS_LOW();
        res = sd_send_cmd(SD_CMD16, 512);
        SD_CS_HIGH();
        sd_spi_rw_byte(0xFF);

        if (res != 0) {
            return SD_ERROR;
        }
    }

    if (sd_card_type == SD_TYPE_UNKNOWN) {
        return SD_ERROR_UNSUPPORTED;
    }

    /* 切换到高速模式 */
    sd_spi_set_speed(1);

    sd_initialized = 1;
    return SD_OK;
}

/**
 * @brief SD卡反初始化
 */
void bsp_sd_deinit(void)
{
    sd_initialized = 0;
    sd_card_type = SD_TYPE_UNKNOWN;
    SPI_Cmd(SD_SPI, DISABLE);
}

/**
 * @brief 读取单个扇区
 */
sd_result_t bsp_sd_read_sector(uint32_t sector, uint8_t *buf)
{
    uint8_t res;

    if (!sd_initialized || buf == NULL) {
        return SD_ERROR_PARAM;
    }

    /* SDSC卡需要字节地址 */
    if (sd_card_type != SD_TYPE_SDHC_SDXC) {
        sector *= 512;
    }

    SD_CS_LOW();
    res = sd_send_cmd(SD_CMD17, sector);

    if (res == 0) {
        if (sd_receive_data(buf, 512) != 0) {
            res = 0xFF;
        }
    }

    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    return (res == 0) ? SD_OK : SD_ERROR_READ;
}

/**
 * @brief 读取多个扇区
 */
sd_result_t bsp_sd_read_sectors(uint32_t sector, uint8_t *buf, uint32_t count)
{
    uint8_t res;

    if (!sd_initialized || buf == NULL || count == 0) {
        return SD_ERROR_PARAM;
    }

    /* SDSC卡需要字节地址 */
    if (sd_card_type != SD_TYPE_SDHC_SDXC) {
        sector *= 512;
    }

    SD_CS_LOW();

    if (count == 1) {
        res = sd_send_cmd(SD_CMD17, sector);
        if (res == 0) {
            if (sd_receive_data(buf, 512) != 0) {
                res = 0xFF;
            }
        }
    } else {
        res = sd_send_cmd(SD_CMD18, sector);
        if (res == 0) {
            while (count--) {
                if (sd_receive_data(buf, 512) != 0) {
                    res = 0xFF;
                    break;
                }
                buf += 512;
            }
            /* 发送停止命令 */
            sd_send_cmd(SD_CMD12, 0);
        }
    }

    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    return (res == 0) ? SD_OK : SD_ERROR_READ;
}

/**
 * @brief 写入单个扇区
 */
sd_result_t bsp_sd_write_sector(uint32_t sector, const uint8_t *buf)
{
    uint8_t res;

    if (!sd_initialized || buf == NULL) {
        return SD_ERROR_PARAM;
    }

    /* SDSC卡需要字节地址 */
    if (sd_card_type != SD_TYPE_SDHC_SDXC) {
        sector *= 512;
    }

    SD_CS_LOW();
    res = sd_send_cmd(SD_CMD24, sector);

    if (res == 0) {
        if (sd_send_data(buf, SD_DATA_TOKEN_CMD24) != 0) {
            res = 0xFF;
        }
    }

    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    return (res == 0) ? SD_OK : SD_ERROR_WRITE;
}

/**
 * @brief 写入多个扇区
 */
sd_result_t bsp_sd_write_sectors(uint32_t sector, const uint8_t *buf, uint32_t count)
{
    uint8_t res;

    if (!sd_initialized || buf == NULL || count == 0) {
        return SD_ERROR_PARAM;
    }

    /* SDSC卡需要字节地址 */
    if (sd_card_type != SD_TYPE_SDHC_SDXC) {
        sector *= 512;
    }

    SD_CS_LOW();

    if (count == 1) {
        res = sd_send_cmd(SD_CMD24, sector);
        if (res == 0) {
            if (sd_send_data(buf, SD_DATA_TOKEN_CMD24) != 0) {
                res = 0xFF;
            }
        }
    } else {
        /* 预设块数 */
        if (sd_card_type != SD_TYPE_MMC) {
            sd_send_cmd(SD_CMD55, 0);
            sd_send_cmd(SD_CMD23, count);
        }

        res = sd_send_cmd(SD_CMD25, sector);
        if (res == 0) {
            while (count--) {
                if (sd_send_data(buf, SD_DATA_TOKEN_CMD25) != 0) {
                    res = 0xFF;
                    break;
                }
                buf += 512;
            }
            /* 发送停止令牌 */
            sd_send_data(NULL, SD_STOP_TOKEN_CMD25);
        }
    }

    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    return (res == 0) ? SD_OK : SD_ERROR_WRITE;
}

/**
 * @brief 获取SD卡信息
 */
sd_result_t bsp_sd_get_info(sd_info_t *info)
{
    if (!sd_initialized || info == NULL) {
        return SD_ERROR_PARAM;
    }

    info->type = sd_card_type;
    info->capacity = bsp_sd_get_sector_count();
    info->block_size = 512;

    return SD_OK;
}

/**
 * @brief 获取SD卡扇区数
 */
uint32_t bsp_sd_get_sector_count(void)
{
    uint8_t csd[16];
    uint8_t res;
    uint32_t capacity = 0;
    uint8_t n;
    uint16_t csize;

    if (!sd_initialized) {
        return 0;
    }

    /* 读取CSD */
    SD_CS_LOW();
    res = sd_send_cmd(SD_CMD9, 0);
    if (res == 0) {
        if (sd_receive_data(csd, 16) == 0) {
            if ((csd[0] >> 6) == 1) {
                /* CSD V2.0 (SDHC/SDXC) */
                csize = ((uint32_t)csd[7] << 16) | ((uint32_t)csd[8] << 8) | csd[9];
                capacity = (csize + 1) * 1024;  /* 扇区数 */
            } else {
                /* CSD V1.0 */
                n = (csd[5] & 0x0F) + ((csd[10] & 0x80) >> 7) + ((csd[9] & 0x03) << 1) + 2;
                csize = ((csd[6] & 0x03) << 10) | (csd[7] << 2) | ((csd[8] & 0xC0) >> 6);
                capacity = (csize + 1) << (n - 9);  /* 扇区数 */
            }
        }
    }
    SD_CS_HIGH();
    sd_spi_rw_byte(0xFF);

    return capacity;
}

/**
 * @brief 获取SD卡扇区大小
 */
uint16_t bsp_sd_get_sector_size(void)
{
    return 512;
}

/**
 * @brief 获取SD卡块大小
 */
uint32_t bsp_sd_get_block_size(void)
{
    return 1;  /* 以扇区为单位 */
}

/**
 * @brief 检测SD卡是否就绪
 */
uint8_t bsp_sd_is_ready(void)
{
    return sd_initialized;
}

/**
 * @brief 同步
 */
sd_result_t bsp_sd_sync(void)
{
    SD_CS_LOW();
    if (sd_wait_ready(500)) {
        SD_CS_HIGH();
        return SD_OK;
    }
    SD_CS_HIGH();
    return SD_ERROR_TIMEOUT;
}
