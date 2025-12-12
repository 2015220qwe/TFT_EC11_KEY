/**
 * @file ffconf.h
 * @brief FatFS配置文件
 * @note 基于FatFs R0.15 (elm-chan.org)
 * @date 2025-12-12
 */

#ifndef FFCONF_DEF
#define FFCONF_DEF	80286  /* FatFs版本标识 */

/*=============================================================================
 *                              功能配置
 *============================================================================*/

/* 文件系统只读选项 */
#define FF_FS_READONLY      0
/* 0: 读写模式 */
/* 1: 只读模式 (禁用f_write, f_sync, f_unlink等) */

/* 文件系统最小化级别 */
#define FF_FS_MINIMIZE      0
/* 0: 全功能 */
/* 1: 移除 f_stat, f_getfree, f_unlink, f_mkdir, f_truncate, f_rename */
/* 2: 还移除 f_opendir, f_readdir, f_closedir */
/* 3: 还移除 f_lseek */

/* 字符串函数 */
#define FF_USE_STRFUNC      1
/* 0: 禁用字符串函数 */
/* 1: 启用但不带LF-CRLF转换 */
/* 2: 启用带LF-CRLF转换 */

/* f_mkfs() 函数 */
#define FF_USE_MKFS         1
/* 0: 禁用 f_mkfs() */
/* 1: 启用 f_mkfs() */

/* f_fastseek() 函数 */
#define FF_USE_FASTSEEK     0

/* 文件名缓冲使用LFNBUF */
#define FF_USE_FIND         0

/* f_chmod() 和 f_utime() 函数 */
#define FF_USE_CHMOD        0

/* f_getlabel() 和 f_setlabel() 函数 */
#define FF_USE_LABEL        0

/* f_forward() 函数 */
#define FF_USE_FORWARD      0

/* f_expand() 函数 */
#define FF_USE_EXPAND       0

/*=============================================================================
 *                              文件名/路径配置
 *============================================================================*/

/* 代码页 */
#define FF_CODE_PAGE        936
/* 936: 简体中文GBK */
/* 437: US ASCII */
/* 932: 日语Shift-JIS */

/* 长文件名支持 */
#define FF_USE_LFN          1
/* 0: 禁用长文件名 (仅8.3格式) */
/* 1: 静态工作缓冲区 */
/* 2: 栈上动态缓冲区 */
/* 3: 堆上动态缓冲区 */

/* 长文件名缓冲区大小 */
#define FF_MAX_LFN          255

/* 路径名长度 */
#define FF_LFN_UNICODE      0
/* 0: ANSI/OEM */
/* 1: UTF-16 */
/* 2: UTF-8 */
/* 3: UTF-32 */

#define FF_LFN_BUF          255
#define FF_SFN_BUF          12

/* 相对路径 */
#define FF_FS_RPATH         0
/* 0: 禁用 */
/* 1: 启用f_chdir和f_chdrive */
/* 2: 还启用f_getcwd */

/*=============================================================================
 *                              驱动器/卷配置
 *============================================================================*/

/* 逻辑驱动器数量 */
#define FF_VOLUMES          1

/* 卷字符串 */
#define FF_STR_VOLUME_ID    0
#define FF_VOLUME_STRS      "SD"

/* 多分区 */
#define FF_MULTI_PARTITION  0
/* 0: 每个物理驱动器一个卷 */
/* 1: 支持多分区 */

/* 扇区大小范围 */
#define FF_MIN_SS           512
#define FF_MAX_SS           512
/* 如果两者相同,可减少代码 */

/* 64位LBA支持 */
#define FF_LBA64            0

/* exFAT支持 */
#define FF_FS_EXFAT         0
/* 0: 禁用exFAT */
/* 1: 启用exFAT (需要FF_LBA64=1) */

/* TRIM命令支持 */
#define FF_USE_TRIM         0

/*=============================================================================
 *                              系统配置
 *============================================================================*/

/* 同步对象使能 */
#define FF_FS_NORTC         1
/* 0: 使用RTC获取时间戳 */
/* 1: 不使用RTC (时间戳为固定值) */

#define FF_NORTC_MON        12
#define FF_NORTC_MDAY       12
#define FF_NORTC_YEAR       2025

/* 文件锁定 */
#define FF_FS_LOCK          0

/* 重入配置 */
#define FF_FS_REENTRANT     0
#define FF_FS_TIMEOUT       1000

/*=============================================================================*/
#endif /* FFCONF_DEF */
