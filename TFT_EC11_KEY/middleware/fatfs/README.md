# FatFS文件系统模块

## 简介

FatFS是由ChaN开发的通用FAT/exFAT文件系统模块，专为嵌入式系统设计。

官方网站: http://elm-chan.org/fsw/ff/

## 文件说明

本目录已包含：
- `ffconf.h` - FatFS配置文件（已配置好）
- `diskio.h` - 磁盘IO接口定义
- `diskio.c` - 磁盘IO实现（对接SD卡驱动）
- `ff.h` - FatFS头文件（仅类型定义）

## 获取完整FatFS

由于版权原因，完整的FatFS实现文件需要从官方下载：

1. 访问 http://elm-chan.org/fsw/ff/archives.html
2. 下载最新版本（推荐R0.15）
3. 解压后将 `ff.c` 复制到本目录

## 使用方法

```c
#include "middleware/fatfs/ff.h"

FATFS fs;
FIL file;
FRESULT res;
UINT bw, br;
char buffer[128];

// 1. 挂载文件系统
res = f_mount(&fs, "0:", 1);
if (res != FR_OK) {
    // 挂载失败处理
}

// 2. 打开/创建文件
res = f_open(&file, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
if (res == FR_OK) {
    // 写入数据
    f_write(&file, "Hello FatFS!\n", 13, &bw);
    f_close(&file);
}

// 3. 读取文件
res = f_open(&file, "0:/test.txt", FA_READ);
if (res == FR_OK) {
    f_read(&file, buffer, sizeof(buffer) - 1, &br);
    buffer[br] = '\0';
    f_close(&file);
}

// 4. 列出目录
DIR dir;
FILINFO fno;

res = f_opendir(&dir, "0:/");
if (res == FR_OK) {
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;

        if (fno.fattrib & AM_DIR) {
            // 目录
            printf("[DIR] %s\n", fno.fname);
        } else {
            // 文件
            printf("[FILE] %s (%lu bytes)\n", fno.fname, fno.fsize);
        }
    }
    f_closedir(&dir);
}

// 5. 卸载
f_mount(NULL, "0:", 0);
```

## 硬件连接

SD卡模块使用SPI2接口：

```
SD卡模块          开发板
────────          ──────
  CS     ────────   PB12
  SCK    ────────   PB13
  MISO   ────────   PB14
  MOSI   ────────   PB15
  VCC    ────────   3.3V
  GND    ────────   GND
```

## 配置说明

ffconf.h主要配置项：

| 配置项 | 当前值 | 说明 |
|--------|--------|------|
| FF_FS_READONLY | 0 | 读写模式 |
| FF_USE_LFN | 1 | 支持长文件名 |
| FF_CODE_PAGE | 936 | 支持中文GBK |
| FF_VOLUMES | 1 | 1个磁盘卷 |
| FF_USE_MKFS | 1 | 支持格式化 |
| FF_USE_STRFUNC | 1 | 支持字符串函数 |

## 常见问题

### Q1: f_mount返回FR_NOT_READY
- 检查SD卡是否插好
- 检查SPI接线是否正确
- 检查供电是否充足（建议外部供电）

### Q2: 中文文件名乱码
- 确保FF_CODE_PAGE设置为936
- 确保FF_USE_LFN设置为1

### Q3: 写入速度慢
- 尝试使用DMA传输
- 增大SPI时钟频率
- 使用多扇区写入函数

## 参考资源

- [FatFs官方文档](http://elm-chan.org/fsw/ff/00index_e.html)
- [STM32 SD卡教程](https://deepbluembedded.com/stm32-sd-card-spi-fatfs-tutorial-examples/)
- [GitHub示例](https://github.com/eziya/STM32_SPI_SDCARD)
