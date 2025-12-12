/**
 * @file ff.h
 * @brief FatFS文件系统头文件 - 精简定义
 * @note 完整实现请从 http://elm-chan.org/fsw/ff/ 下载
 * @version R0.15
 * @date 2025-12-12
 *
 * @note 使用方法:
 *       1. 从elm-chan.org下载FatFs源码
 *       2. 将ff.c和ff.h复制到本目录
 *       3. 使用本项目的ffconf.h配置
 */

#ifndef FF_DEFINED
#define FF_DEFINED  80286  /* 版本标识 */

#ifdef __cplusplus
extern "C" {
#endif

#include "ffconf.h"
#include <stdint.h>

/*=============================================================================
 *                              类型定义
 *============================================================================*/

/* FATFS基本类型 */
typedef uint8_t     BYTE;
typedef uint16_t    WORD;
typedef uint32_t    DWORD;
typedef uint64_t    QWORD;
typedef WORD        WCHAR;

#if FF_FS_EXFAT
typedef QWORD       FSIZE_t;
typedef QWORD       LBA_t;
#else
typedef DWORD       FSIZE_t;
typedef DWORD       LBA_t;
#endif

/* 文件系统对象结构 */
typedef struct {
    BYTE    fs_type;        /* 文件系统类型 (0:未挂载) */
    BYTE    pdrv;           /* 物理驱动器号 */
    BYTE    n_fats;         /* FAT表数量 (1或2) */
    BYTE    wflag;          /* 写标志 */
    BYTE    fsi_flag;       /* FSINFO标志 */
    WORD    id;             /* 卷挂载ID */
    WORD    n_rootdir;      /* 根目录项数 (FAT12/16) */
    WORD    csize;          /* 簇大小 [扇区] */
#if FF_MAX_SS != FF_MIN_SS
    WORD    ssize;          /* 扇区大小 */
#endif
#if FF_USE_LFN
    WCHAR*  lfnbuf;         /* LFN工作缓冲区 */
#endif
#if FF_FS_EXFAT
    BYTE*   dirbuf;         /* 目录项缓冲区 */
#endif
#if FF_FS_REENTRANT
    void*   sobj;           /* 同步对象标识符 */
#endif
#if !FF_FS_READONLY
    DWORD   last_clst;      /* 最后分配的簇 */
    DWORD   free_clst;      /* 空闲簇数 */
#endif
#if FF_FS_RPATH
    DWORD   cdir;           /* 当前目录起始簇 (0:根目录) */
#if FF_FS_EXFAT
    DWORD   cdc_scl;
    DWORD   cdc_size;
    DWORD   cdc_ofs;
#endif
#endif
    DWORD   n_fatent;       /* FAT表项数量 */
    DWORD   fsize;          /* FAT大小 [扇区] */
    LBA_t   volbase;        /* 卷基址 */
    LBA_t   fatbase;        /* FAT基址 */
    LBA_t   dirbase;        /* 根目录基址 */
    LBA_t   database;       /* 数据区基址 */
#if FF_FS_EXFAT
    LBA_t   bitbase;
#endif
    LBA_t   winsect;        /* 窗口扇区号 */
    BYTE    win[FF_MAX_SS]; /* 扇区窗口缓冲区 */
} FATFS;

/* 文件对象结构 */
typedef struct {
    FATFS*  fs;             /* 所属文件系统对象指针 */
    WORD    id;             /* 所属文件系统挂载ID */
    BYTE    flag;           /* 状态标志 */
    BYTE    err;            /* 错误代码 */
    FSIZE_t fptr;           /* 文件读写指针 */
    DWORD   clust;          /* 当前簇 */
    LBA_t   sect;           /* 当前数据扇区 */
#if !FF_FS_READONLY
    LBA_t   dir_sect;       /* 目录项所在扇区 */
    BYTE*   dir_ptr;        /* 目录项指针 */
#endif
#if FF_USE_FASTSEEK
    DWORD*  cltbl;          /* 簇链表指针 */
#endif
#if !FF_FS_TINY
    BYTE    buf[FF_MAX_SS]; /* 文件私有缓冲区 */
#endif
    DWORD   obj_size;       /* 对象大小 (目录/文件) */
    DWORD   sclust;         /* 起始簇 (0:根目录) */
#if FF_FS_EXFAT
    DWORD   objsize;
    DWORD   n_cont;
    DWORD   n_frag;
    DWORD   c_scl;
    DWORD   c_size;
    DWORD   c_ofs;
    BYTE    stat;
#endif
} FIL;

/* 目录对象结构 */
typedef struct {
    FATFS*  fs;             /* 所属文件系统对象指针 */
    WORD    id;             /* 所属文件系统挂载ID */
    WORD    index;          /* 当前读取索引 */
    DWORD   sclust;         /* 表起始簇 */
    LBA_t   sect;           /* 当前扇区 */
    BYTE*   dir;            /* 当前扇区SFN入口指针 */
    BYTE    fn[12];         /* SFN缓冲区 */
#if FF_USE_LFN
    DWORD   blk_ofs;        /* LFN块偏移 */
#endif
#if FF_USE_FIND
    const TCHAR* pat;       /* 匹配模式指针 */
#endif
#if FF_FS_EXFAT
    DWORD   objsize;
    BYTE    stat;
#endif
} DIR;

/* 文件信息结构 */
typedef struct {
    FSIZE_t fsize;          /* 文件大小 */
    WORD    fdate;          /* 最后修改日期 */
    WORD    ftime;          /* 最后修改时间 */
    BYTE    fattrib;        /* 属性 */
#if FF_USE_LFN
    TCHAR   altname[FF_SFN_BUF + 1];  /* 短文件名 */
    TCHAR   fname[FF_LFN_BUF + 1];    /* 长文件名 */
#else
    TCHAR   fname[12 + 1];  /* 短文件名 */
#endif
} FILINFO;

/* 文件函数返回值 */
typedef enum {
    FR_OK = 0,              /* 成功 */
    FR_DISK_ERR,            /* 磁盘I/O层发生错误 */
    FR_INT_ERR,             /* 断言失败 */
    FR_NOT_READY,           /* 物理驱动器无法工作 */
    FR_NO_FILE,             /* 找不到文件 */
    FR_NO_PATH,             /* 找不到路径 */
    FR_INVALID_NAME,        /* 路径名格式无效 */
    FR_DENIED,              /* 由于禁止访问或目录已满而拒绝访问 */
    FR_EXIST,               /* 由于禁止访问而拒绝访问 */
    FR_INVALID_OBJECT,      /* 文件/目录对象无效 */
    FR_WRITE_PROTECTED,     /* 物理驱动器写保护 */
    FR_INVALID_DRIVE,       /* 逻辑驱动器号无效 */
    FR_NOT_ENABLED,         /* 卷没有工作区 */
    FR_NO_FILESYSTEM,       /* 没有有效的FAT卷 */
    FR_MKFS_ABORTED,        /* f_mkfs()由于任何问题而中止 */
    FR_TIMEOUT,             /* 无法在定义的时间内获得授权访问卷 */
    FR_LOCKED,              /* 操作被文件共享控制拒绝 */
    FR_NOT_ENOUGH_CORE,     /* 无法分配LFN工作缓冲区 */
    FR_TOO_MANY_OPEN_FILES, /* 打开的文件数 > FF_FS_LOCK */
    FR_INVALID_PARAMETER    /* 给定参数无效 */
} FRESULT;

/* 文件访问模式和属性 */
#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_OPEN_EXISTING    0x00
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30

/* 文件属性 */
#define AM_RDO      0x01    /* 只读 */
#define AM_HID      0x02    /* 隐藏 */
#define AM_SYS      0x04    /* 系统 */
#define AM_DIR      0x10    /* 目录 */
#define AM_ARC      0x20    /* 归档 */

/*=============================================================================
 *                              字符类型定义
 *============================================================================*/

#if FF_USE_LFN && FF_LFN_UNICODE == 1
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#elif FF_USE_LFN && FF_LFN_UNICODE == 2
typedef char TCHAR;
#define _T(x) u8 ## x
#define _TEXT(x) u8 ## x
#elif FF_USE_LFN && FF_LFN_UNICODE == 3
typedef DWORD TCHAR;
#define _T(x) U ## x
#define _TEXT(x) U ## x
#else
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif

/*=============================================================================
 *                              函数声明
 *============================================================================*/

/* 挂载/卸载 */
FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt);

/* 文件操作 */
FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode);
FRESULT f_close (FIL* fp);
FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_lseek (FIL* fp, FSIZE_t ofs);
FRESULT f_sync (FIL* fp);
FRESULT f_truncate (FIL* fp);

/* 目录操作 */
FRESULT f_opendir (DIR* dp, const TCHAR* path);
FRESULT f_closedir (DIR* dp);
FRESULT f_readdir (DIR* dp, FILINFO* fno);
FRESULT f_mkdir (const TCHAR* path);
FRESULT f_unlink (const TCHAR* path);
FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new);
FRESULT f_stat (const TCHAR* path, FILINFO* fno);
FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask);
FRESULT f_utime (const TCHAR* path, const FILINFO* fno);
FRESULT f_chdir (const TCHAR* path);
FRESULT f_getcwd (TCHAR* buff, UINT len);

/* 格式化和卷信息 */
FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);
FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);
FRESULT f_setlabel (const TCHAR* label);
FRESULT f_mkfs (const TCHAR* path, const void* opt, void* work, UINT len);

/* 字符串操作 */
#if FF_USE_STRFUNC
int f_putc (TCHAR c, FIL* fp);
int f_puts (const TCHAR* str, FIL* cp);
int f_printf (FIL* fp, const TCHAR* fmt, ...);
TCHAR* f_gets (TCHAR* buff, int len, FIL* fp);
#endif

/* 文件指针宏 */
#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj_size))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj_size)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)
#define f_unmount(path) f_mount(0, path, 0)

#ifdef __cplusplus
}
#endif

#endif /* FF_DEFINED */
