/**
 * @file ff.c
 * @brief FatFS文件系统核心实现 - 精简版
 * @note 基于FatFs by ChaN (elm-chan.org)
 * @note 本版本为精简实现，完整版请从官网下载
 * @version R0.15 (simplified)
 * @date 2025-12-12
 */

#include "ff.h"
#include "diskio.h"
#include <string.h>

/*=============================================================================
 *                              私有宏定义
 *============================================================================*/

#define ABORT(fs, res)      { fp->err = (BYTE)(res); return res; }
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define SS(fs)              ((UINT)FF_MAX_SS)
#define LD_WORD(ptr)        (WORD)(((WORD)*((BYTE*)(ptr)+1)<<8)|(WORD)*(BYTE*)(ptr))
#define LD_DWORD(ptr)       (DWORD)(((DWORD)*((BYTE*)(ptr)+3)<<24)|((DWORD)*((BYTE*)(ptr)+2)<<16)|((WORD)*((BYTE*)(ptr)+1)<<8)|*(BYTE*)(ptr))
#define ST_WORD(ptr,val)    *(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8)
#define ST_DWORD(ptr,val)   *(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8); *((BYTE*)(ptr)+2)=(BYTE)((DWORD)(val)>>16); *((BYTE*)(ptr)+3)=(BYTE)((DWORD)(val)>>24)

/* FAT类型 */
#define FS_FAT12    1
#define FS_FAT16    2
#define FS_FAT32    3

/* 目录项属性 */
#define DIR_Name        0
#define DIR_Attr        11
#define DIR_NTres       12
#define DIR_CrtTime     14
#define DIR_CrtDate     16
#define DIR_FstClusHI   20
#define DIR_WrtTime     22
#define DIR_WrtDate     24
#define DIR_FstClusLO   26
#define DIR_FileSize    28
#define DDEM            0xE5
#define SZ_DIRE         32

/* BPB偏移 */
#define BS_jmpBoot      0
#define BS_OEMName      3
#define BPB_BytsPerSec  11
#define BPB_SecPerClus  13
#define BPB_RsvdSecCnt  14
#define BPB_NumFATs     16
#define BPB_RootEntCnt  17
#define BPB_TotSec16    19
#define BPB_Media       21
#define BPB_FATSz16     22
#define BPB_SecPerTrk   24
#define BPB_NumHeads    26
#define BPB_HiddSec     28
#define BPB_TotSec32    32
#define BS_DrvNum       36
#define BS_BootSig      38
#define BS_VolID        39
#define BS_VolLab       43
#define BS_FilSysType   54
#define BPB_FATSz32     36
#define BPB_ExtFlags32  40
#define BPB_FSVer32     42
#define BPB_RootClus32  44
#define BPB_FSInfo32    48
#define BPB_BkBootSec32 50
#define BS_DrvNum32     64
#define BS_BootSig32    66
#define BS_VolID32      67
#define BS_VolLab32     71
#define BS_FilSysType32 82
#define FSI_LeadSig     0
#define FSI_StrucSig    484
#define FSI_Free_Count  488
#define FSI_Nxt_Free    492
#define MBR_Table       446

/*=============================================================================
 *                              静态变量
 *============================================================================*/

static FATFS *FatFs[FF_VOLUMES];
static WORD Fsid;

#if FF_USE_LFN == 1
static WCHAR LfnBuf[FF_MAX_LFN + 1];
#endif

/*=============================================================================
 *                              私有函数
 *============================================================================*/

static FRESULT sync_window(FATFS *fs)
{
    DRESULT res = RES_OK;
    if (fs->wflag) {
        res = disk_write(fs->pdrv, fs->win, (LBA_t)fs->winsect, 1);
        fs->wflag = 0;
        if (res != RES_OK) return FR_DISK_ERR;
    }
    return FR_OK;
}

static FRESULT move_window(FATFS *fs, LBA_t sect)
{
    FRESULT res = FR_OK;
    if (sect != fs->winsect) {
        res = sync_window(fs);
        if (res == FR_OK) {
            if (disk_read(fs->pdrv, fs->win, sect, 1) != RES_OK) {
                sect = (LBA_t)0 - 1;
                res = FR_DISK_ERR;
            }
            fs->winsect = sect;
        }
    }
    return res;
}

static DWORD get_fat(FATFS *fs, DWORD clst)
{
    UINT wc, bc;
    DWORD val;
    LBA_t sect;

    if (clst < 2 || clst >= fs->n_fatent) return 1;

    switch (fs->fs_type) {
    case FS_FAT12:
        bc = (UINT)clst; bc += bc / 2;
        sect = fs->fatbase + (bc / SS(fs));
        if (move_window(fs, sect) != FR_OK) return 1;
        wc = fs->win[bc++ % SS(fs)];
        if (move_window(fs, fs->fatbase + (bc / SS(fs))) != FR_OK) return 1;
        wc |= fs->win[bc % SS(fs)] << 8;
        val = (clst & 1) ? (wc >> 4) : (wc & 0xFFF);
        break;

    case FS_FAT16:
        sect = fs->fatbase + (clst / (SS(fs) / 2));
        if (move_window(fs, sect) != FR_OK) return 1;
        val = LD_WORD(&fs->win[(clst * 2) % SS(fs)]);
        break;

    case FS_FAT32:
        sect = fs->fatbase + (clst / (SS(fs) / 4));
        if (move_window(fs, sect) != FR_OK) return 1;
        val = LD_DWORD(&fs->win[(clst * 4) % SS(fs)]) & 0x0FFFFFFF;
        break;

    default:
        val = 1;
    }
    return val;
}

#if !FF_FS_READONLY
static FRESULT put_fat(FATFS *fs, DWORD clst, DWORD val)
{
    UINT bc;
    BYTE *p;
    LBA_t sect;
    FRESULT res = FR_INT_ERR;

    if (clst >= 2 && clst < fs->n_fatent) {
        switch (fs->fs_type) {
        case FS_FAT12:
            bc = (UINT)clst; bc += bc / 2;
            sect = fs->fatbase + (bc / SS(fs));
            res = move_window(fs, sect);
            if (res != FR_OK) break;
            p = fs->win + bc++ % SS(fs);
            *p = (clst & 1) ? ((*p & 0x0F) | ((BYTE)val << 4)) : (BYTE)val;
            fs->wflag = 1;
            sect = fs->fatbase + (bc / SS(fs));
            res = move_window(fs, sect);
            if (res != FR_OK) break;
            p = fs->win + bc % SS(fs);
            *p = (clst & 1) ? (BYTE)(val >> 4) : ((*p & 0xF0) | ((BYTE)(val >> 8) & 0x0F));
            fs->wflag = 1;
            break;

        case FS_FAT16:
            sect = fs->fatbase + (clst / (SS(fs) / 2));
            res = move_window(fs, sect);
            if (res != FR_OK) break;
            ST_WORD(&fs->win[(clst * 2) % SS(fs)], (WORD)val);
            fs->wflag = 1;
            break;

        case FS_FAT32:
            sect = fs->fatbase + (clst / (SS(fs) / 4));
            res = move_window(fs, sect);
            if (res != FR_OK) break;
            val = (val & 0x0FFFFFFF) | (LD_DWORD(&fs->win[(clst * 4) % SS(fs)]) & 0xF0000000);
            ST_DWORD(&fs->win[(clst * 4) % SS(fs)], val);
            fs->wflag = 1;
            break;
        }
    }
    return res;
}
#endif

static LBA_t clst2sect(FATFS *fs, DWORD clst)
{
    clst -= 2;
    if (clst >= fs->n_fatent - 2) return 0;
    return fs->database + (LBA_t)fs->csize * clst;
}

static FRESULT check_fs(FATFS *fs, LBA_t sect)
{
    fs->wflag = 0; fs->winsect = (LBA_t)0 - 1;
    if (move_window(fs, sect) != FR_OK) return FR_DISK_ERR;
    if (LD_WORD(&fs->win[BS_BootSig - 2]) != 0xAA55) return FR_NO_FILESYSTEM;
    if ((LD_DWORD(&fs->win[BS_FilSysType]) & 0xFFFFFF) == 0x544146) return FR_OK;
    if ((LD_DWORD(&fs->win[BS_FilSysType32]) & 0xFFFFFF) == 0x544146) return FR_OK;
    return FR_NO_FILESYSTEM;
}

static UINT find_volume(FATFS *fs, BYTE part)
{
    UINT i;
    DWORD br[4];

    if (part) {
        for (i = 0; i < 4; i++) {
            br[i] = part == i + 1 ? LD_DWORD(&fs->win[MBR_Table + i * 16 + 8]) : 0;
        }
        i = part - 1;
        return (UINT)br[i];
    }
    if (fs->win[MBR_Table + 4]) {
        for (i = 0; i < 4; i++) {
            if (fs->win[MBR_Table + i * 16 + 4] == 0x0B ||
                fs->win[MBR_Table + i * 16 + 4] == 0x0C ||
                fs->win[MBR_Table + i * 16 + 4] == 0x06 ||
                fs->win[MBR_Table + i * 16 + 4] == 0x04 ||
                fs->win[MBR_Table + i * 16 + 4] == 0x01) {
                return (UINT)LD_DWORD(&fs->win[MBR_Table + i * 16 + 8]);
            }
        }
    }
    return 0;
}

/*=============================================================================
 *                              公共函数实现
 *============================================================================*/

FRESULT f_mount(FATFS *fs, const TCHAR *path, BYTE opt)
{
    FATFS *cfs;
    int vol;
    FRESULT res;
    BYTE fmt;
    DWORD bsect, farone, fatbase, tsect, sysect, nclst, szbfat;
    WORD nrsv;

    vol = (int)(path[0] - '0');
    if (vol < 0 || vol >= FF_VOLUMES) return FR_INVALID_DRIVE;

    cfs = FatFs[vol];
    if (cfs) {
        cfs->fs_type = 0;
    }

    if (fs) {
        fs->fs_type = 0;
        fs->pdrv = (BYTE)vol;
#if FF_USE_LFN == 1
        fs->lfnbuf = LfnBuf;
#endif
    }
    FatFs[vol] = fs;

    if (opt == 0) return FR_OK;
    if (!fs) return FR_INVALID_OBJECT;

    /* 初始化磁盘 */
    if (disk_initialize(fs->pdrv) & STA_NOINIT) {
        return FR_NOT_READY;
    }

    /* 查找FAT卷 */
    bsect = 0;
    fmt = check_fs(fs, bsect);
    if (fmt == FR_NO_FILESYSTEM) {
        if (disk_read(fs->pdrv, fs->win, 0, 1) != RES_OK) return FR_DISK_ERR;
        bsect = find_volume(fs, 0);
        if (bsect) {
            fmt = check_fs(fs, bsect);
        }
    }
    if (fmt != FR_OK) return fmt;

    /* 解析BPB */
    if (LD_WORD(&fs->win[BPB_BytsPerSec]) != SS(fs)) return FR_NO_FILESYSTEM;

    farone = LD_WORD(&fs->win[BPB_FATSz16]);
    if (farone == 0) farone = LD_DWORD(&fs->win[BPB_FATSz32]);
    fs->fsize = farone;

    fs->n_fats = fs->win[BPB_NumFATs];
    if (fs->n_fats != 1 && fs->n_fats != 2) return FR_NO_FILESYSTEM;
    farone *= fs->n_fats;

    fs->csize = fs->win[BPB_SecPerClus];
    if (fs->csize == 0 || (fs->csize & (fs->csize - 1))) return FR_NO_FILESYSTEM;

    fs->n_rootdir = LD_WORD(&fs->win[BPB_RootEntCnt]);
    if (fs->n_rootdir % (SS(fs) / SZ_DIRE)) return FR_NO_FILESYSTEM;

    tsect = LD_WORD(&fs->win[BPB_TotSec16]);
    if (tsect == 0) tsect = LD_DWORD(&fs->win[BPB_TotSec32]);

    nrsv = LD_WORD(&fs->win[BPB_RsvdSecCnt]);
    if (nrsv == 0) return FR_NO_FILESYSTEM;

    fatbase = bsect + nrsv;
    sysect = nrsv + farone + fs->n_rootdir / (SS(fs) / SZ_DIRE);
    if (tsect < sysect) return FR_NO_FILESYSTEM;
    nclst = (tsect - sysect) / fs->csize;
    if (nclst == 0) return FR_NO_FILESYSTEM;

    /* 确定FAT类型 */
    fmt = 0;
    if (nclst <= 0xFF5) fmt = FS_FAT12;
    else if (nclst <= 0xFFF5) fmt = FS_FAT16;
    else fmt = FS_FAT32;

    fs->fs_type = fmt;
    fs->n_fatent = nclst + 2;
    fs->volbase = bsect;
    fs->fatbase = fatbase;
    fs->dirbase = fatbase + farone;
    fs->database = fatbase + farone + fs->n_rootdir / (SS(fs) / SZ_DIRE);

    if (fmt == FS_FAT32) {
        fs->dirbase = LD_DWORD(&fs->win[BPB_RootClus32]);
    }

#if !FF_FS_READONLY
    fs->last_clst = fs->free_clst = 0xFFFFFFFF;
#endif
    fs->id = ++Fsid;
    fs->wflag = 0;

    return FR_OK;
}

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode)
{
    FRESULT res;
    FATFS *fs;
    DIR dj;
    BYTE *dir;
    LBA_t sect;
    DWORD cl, clst;
    FSIZE_t ofs;
    int vol;

    if (!fp) return FR_INVALID_OBJECT;
    fp->fs = 0;

    vol = (int)(path[0] - '0');
    if (vol < 0 || vol >= FF_VOLUMES) return FR_INVALID_DRIVE;
    if (path[1] != ':') return FR_INVALID_NAME;
    path += 2;
    if (*path == '/' || *path == '\\') path++;

    fs = FatFs[vol];
    if (!fs || !fs->fs_type) return FR_NOT_ENABLED;

    /* 简化: 仅处理根目录文件 */
    memset(&dj, 0, sizeof(dj));
    dj.fs = fs;
    dj.sclust = (fs->fs_type == FS_FAT32) ? (DWORD)fs->dirbase : 0;

    /* 在根目录中查找文件 */
    if (fs->fs_type == FS_FAT32) {
        sect = clst2sect(fs, (DWORD)fs->dirbase);
    } else {
        sect = fs->dirbase;
    }

    res = move_window(fs, sect);
    if (res != FR_OK) return res;

    /* 搜索目录项 */
    UINT i;
    for (i = 0; i < fs->n_rootdir || (fs->fs_type == FS_FAT32 && i < 512); i++) {
        dir = fs->win + (i % (SS(fs) / SZ_DIRE)) * SZ_DIRE;
        if (i > 0 && (i % (SS(fs) / SZ_DIRE)) == 0) {
            sect++;
            res = move_window(fs, sect);
            if (res != FR_OK) return res;
            dir = fs->win;
        }

        if (dir[DIR_Name] == 0) break;
        if (dir[DIR_Name] == DDEM) continue;
        if (dir[DIR_Attr] & AM_DIR) continue;

        /* 比较文件名 (简化: 仅8.3格式) */
        BYTE sfn[11];
        memset(sfn, ' ', 11);
        int j = 0, k = 0;
        while (path[j] && path[j] != '.' && k < 8) {
            sfn[k++] = (path[j] >= 'a' && path[j] <= 'z') ? path[j] - 32 : path[j];
            j++;
        }
        if (path[j] == '.') {
            j++;
            k = 8;
            while (path[j] && k < 11) {
                sfn[k++] = (path[j] >= 'a' && path[j] <= 'z') ? path[j] - 32 : path[j];
                j++;
            }
        }

        if (memcmp(dir, sfn, 11) == 0) {
            /* 找到文件 */
            fp->fs = fs;
            fp->id = fs->id;
            fp->flag = mode;
            fp->err = 0;
            fp->fptr = 0;
            fp->obj_size = LD_DWORD(&dir[DIR_FileSize]);
            fp->sclust = ((DWORD)LD_WORD(&dir[DIR_FstClusHI]) << 16) | LD_WORD(&dir[DIR_FstClusLO]);
            fp->clust = fp->sclust;
#if !FF_FS_READONLY
            fp->dir_sect = sect;
            fp->dir_ptr = dir;
#endif
            return FR_OK;
        }
    }

#if !FF_FS_READONLY
    /* 文件不存在，创建新文件 */
    if (mode & (FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS)) {
        /* 查找空目录项 */
        if (fs->fs_type == FS_FAT32) {
            sect = clst2sect(fs, (DWORD)fs->dirbase);
        } else {
            sect = fs->dirbase;
        }
        res = move_window(fs, sect);
        if (res != FR_OK) return res;

        for (i = 0; i < fs->n_rootdir || (fs->fs_type == FS_FAT32 && i < 512); i++) {
            dir = fs->win + (i % (SS(fs) / SZ_DIRE)) * SZ_DIRE;
            if (i > 0 && (i % (SS(fs) / SZ_DIRE)) == 0) {
                sect++;
                res = move_window(fs, sect);
                if (res != FR_OK) return res;
                dir = fs->win;
            }

            if (dir[DIR_Name] == 0 || dir[DIR_Name] == DDEM) {
                /* 创建目录项 */
                memset(dir, 0, SZ_DIRE);

                /* 设置文件名 */
                BYTE sfn[11];
                memset(sfn, ' ', 11);
                int j = 0, k = 0;
                while (path[j] && path[j] != '.' && k < 8) {
                    sfn[k++] = (path[j] >= 'a' && path[j] <= 'z') ? path[j] - 32 : path[j];
                    j++;
                }
                if (path[j] == '.') {
                    j++;
                    k = 8;
                    while (path[j] && k < 11) {
                        sfn[k++] = (path[j] >= 'a' && path[j] <= 'z') ? path[j] - 32 : path[j];
                        j++;
                    }
                }
                memcpy(dir, sfn, 11);

                dir[DIR_Attr] = AM_ARC;
                DWORD tm = get_fattime();
                ST_DWORD(&dir[DIR_WrtTime], tm);
                ST_WORD(&dir[DIR_FstClusLO], 0);
                ST_WORD(&dir[DIR_FstClusHI], 0);
                ST_DWORD(&dir[DIR_FileSize], 0);

                fs->wflag = 1;
                res = sync_window(fs);
                if (res != FR_OK) return res;

                fp->fs = fs;
                fp->id = fs->id;
                fp->flag = mode;
                fp->err = 0;
                fp->fptr = 0;
                fp->obj_size = 0;
                fp->sclust = 0;
                fp->clust = 0;
                fp->dir_sect = sect;
                fp->dir_ptr = dir;

                return FR_OK;
            }
        }
        return FR_DENIED;
    }
#endif

    return FR_NO_FILE;
}

FRESULT f_close(FIL *fp)
{
    FRESULT res;

    if (!fp || !fp->fs) return FR_INVALID_OBJECT;

#if !FF_FS_READONLY
    res = f_sync(fp);
#else
    res = FR_OK;
#endif
    fp->fs = 0;
    return res;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    FRESULT res;
    FATFS *fs;
    DWORD clst;
    LBA_t sect;
    UINT rcnt, cc, csect;
    BYTE *rbuff = (BYTE*)buff;

    *br = 0;
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    if (fp->err) return FR_INT_ERR;
    if (!(fp->flag & FA_READ)) return FR_DENIED;

    fs = fp->fs;

    while (btr > 0) {
        if (fp->fptr >= fp->obj_size) break;

        /* 获取当前簇 */
        if (fp->fptr == 0) {
            fp->clust = fp->sclust;
        } else if ((fp->fptr % (fs->csize * SS(fs))) == 0) {
            clst = get_fat(fs, fp->clust);
            if (clst < 2 || clst >= fs->n_fatent) ABORT(fs, FR_INT_ERR);
            fp->clust = clst;
        }

        sect = clst2sect(fs, fp->clust);
        if (sect == 0) ABORT(fs, FR_INT_ERR);

        csect = (UINT)(fp->fptr / SS(fs)) % fs->csize;
        sect += csect;

        /* 读取数据 */
        cc = btr / SS(fs);
        if (cc > 0) {
            if (csect + cc > fs->csize) cc = fs->csize - csect;
            if (disk_read(fs->pdrv, rbuff, sect, cc) != RES_OK) ABORT(fs, FR_DISK_ERR);
            rcnt = cc * SS(fs);
            if (fp->fptr + rcnt > fp->obj_size) rcnt = (UINT)(fp->obj_size - fp->fptr);
        } else {
            res = move_window(fs, sect);
            if (res != FR_OK) ABORT(fs, res);
            rcnt = SS(fs) - (UINT)(fp->fptr % SS(fs));
            if (rcnt > btr) rcnt = btr;
            if (fp->fptr + rcnt > fp->obj_size) rcnt = (UINT)(fp->obj_size - fp->fptr);
            memcpy(rbuff, fs->win + (fp->fptr % SS(fs)), rcnt);
        }

        fp->fptr += rcnt;
        rbuff += rcnt;
        btr -= rcnt;
        *br += rcnt;
    }

    return FR_OK;
}

#if !FF_FS_READONLY
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    FRESULT res;
    FATFS *fs;
    DWORD clst, scl;
    LBA_t sect;
    UINT wcnt, cc, csect;
    const BYTE *wbuff = (const BYTE*)buff;

    *bw = 0;
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    if (fp->err) return FR_INT_ERR;
    if (!(fp->flag & FA_WRITE)) return FR_DENIED;

    fs = fp->fs;

    while (btw > 0) {
        /* 获取或分配簇 */
        if (fp->fptr == 0 && fp->sclust == 0) {
            /* 分配第一个簇 */
            scl = fs->last_clst;
            if (scl < 2 || scl >= fs->n_fatent) scl = 2;
            clst = scl;
            do {
                if (clst >= fs->n_fatent) {
                    clst = 2;
                    if (clst == scl) ABORT(fs, FR_DENIED);
                }
                DWORD fat = get_fat(fs, clst);
                if (fat == 0) break;
                clst++;
            } while (1);

            res = put_fat(fs, clst, 0x0FFFFFFF);
            if (res != FR_OK) ABORT(fs, res);
            fp->sclust = fp->clust = clst;
            fs->last_clst = clst;
        } else if ((fp->fptr % (fs->csize * SS(fs))) == 0) {
            if (fp->fptr == 0) {
                fp->clust = fp->sclust;
            } else {
                /* 需要新簇 */
                clst = get_fat(fs, fp->clust);
                if (clst < 2) {
                    /* 分配新簇 */
                    scl = fp->clust + 1;
                    if (scl >= fs->n_fatent) scl = 2;
                    DWORD ncl = scl;
                    do {
                        if (ncl >= fs->n_fatent) {
                            ncl = 2;
                            if (ncl == scl) ABORT(fs, FR_DENIED);
                        }
                        DWORD fat = get_fat(fs, ncl);
                        if (fat == 0) break;
                        ncl++;
                    } while (1);

                    res = put_fat(fs, ncl, 0x0FFFFFFF);
                    if (res != FR_OK) ABORT(fs, res);
                    res = put_fat(fs, fp->clust, ncl);
                    if (res != FR_OK) ABORT(fs, res);

                    clst = ncl;
                    fs->last_clst = ncl;
                }
                fp->clust = clst;
            }
        }

        sect = clst2sect(fs, fp->clust);
        if (sect == 0) ABORT(fs, FR_INT_ERR);

        csect = (UINT)(fp->fptr / SS(fs)) % fs->csize;
        sect += csect;

        /* 写入数据 */
        cc = btw / SS(fs);
        if (cc > 0) {
            if (csect + cc > fs->csize) cc = fs->csize - csect;
            if (disk_write(fs->pdrv, wbuff, sect, cc) != RES_OK) ABORT(fs, FR_DISK_ERR);
            wcnt = cc * SS(fs);
        } else {
            res = move_window(fs, sect);
            if (res != FR_OK) ABORT(fs, res);
            wcnt = SS(fs) - (UINT)(fp->fptr % SS(fs));
            if (wcnt > btw) wcnt = btw;
            memcpy(fs->win + (fp->fptr % SS(fs)), wbuff, wcnt);
            fs->wflag = 1;
        }

        fp->fptr += wcnt;
        if (fp->fptr > fp->obj_size) fp->obj_size = fp->fptr;
        wbuff += wcnt;
        btw -= wcnt;
        *bw += wcnt;
    }

    fp->flag |= 0x40;  /* 已修改 */
    return FR_OK;
}

FRESULT f_sync(FIL *fp)
{
    FRESULT res;
    FATFS *fs;
    DWORD tm;
    BYTE *dir;

    if (!fp || !fp->fs) return FR_INVALID_OBJECT;

    fs = fp->fs;

    if (fp->flag & 0x40) {
        /* 更新目录项 */
        res = move_window(fs, fp->dir_sect);
        if (res != FR_OK) return res;

        dir = fp->dir_ptr;
        dir[DIR_Attr] |= AM_ARC;
        ST_DWORD(&dir[DIR_FileSize], fp->obj_size);
        ST_WORD(&dir[DIR_FstClusLO], fp->sclust);
        ST_WORD(&dir[DIR_FstClusHI], fp->sclust >> 16);
        tm = get_fattime();
        ST_DWORD(&dir[DIR_WrtTime], tm);
        fs->wflag = 1;

        fp->flag &= ~0x40;
    }

    res = sync_window(fs);
    return res;
}
#endif

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    FRESULT res;
    FATFS *fs;
    DWORD clst;
    FSIZE_t ifptr;

    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    if (fp->err) return FR_INT_ERR;

    fs = fp->fs;

    if (ofs > fp->obj_size && !(fp->flag & FA_WRITE)) {
        ofs = fp->obj_size;
    }

    ifptr = fp->fptr;
    fp->fptr = 0;

    if (ofs > 0) {
        DWORD bcs = (DWORD)fs->csize * SS(fs);
        if (ifptr > 0 && (ofs - 1) / bcs >= (ifptr - 1) / bcs) {
            fp->fptr = (ifptr - 1) & ~((FSIZE_t)bcs - 1);
            ofs -= fp->fptr;
        }

        clst = fp->sclust;
        if (clst == 0) {
            if (ofs > 0) return FR_INT_ERR;
            fp->clust = clst;
        } else {
            if (fp->fptr == 0) {
                fp->clust = clst;
            }

            while (ofs > bcs) {
                clst = get_fat(fs, fp->clust);
                if (clst < 2 || clst >= fs->n_fatent) {
                    if (!(fp->flag & FA_WRITE)) return FR_INT_ERR;
                    break;
                }
                fp->clust = clst;
                fp->fptr += bcs;
                ofs -= bcs;
            }
            fp->fptr += ofs;
        }
    }

    return FR_OK;
}

FRESULT f_opendir(DIR *dp, const TCHAR *path)
{
    FRESULT res;
    FATFS *fs;
    int vol;

    if (!dp) return FR_INVALID_OBJECT;

    vol = (int)(path[0] - '0');
    if (vol < 0 || vol >= FF_VOLUMES) return FR_INVALID_DRIVE;

    fs = FatFs[vol];
    if (!fs || !fs->fs_type) return FR_NOT_ENABLED;

    dp->fs = fs;
    dp->id = fs->id;
    dp->index = 0;
    dp->sclust = (fs->fs_type == FS_FAT32) ? (DWORD)fs->dirbase : 0;

    return FR_OK;
}

FRESULT f_closedir(DIR *dp)
{
    if (!dp || !dp->fs) return FR_INVALID_OBJECT;
    dp->fs = 0;
    return FR_OK;
}

FRESULT f_readdir(DIR *dp, FILINFO *fno)
{
    FRESULT res;
    FATFS *fs;
    LBA_t sect;
    BYTE *dir;
    UINT i;

    if (!dp || !dp->fs) return FR_INVALID_OBJECT;
    if (!fno) {
        dp->index = 0;
        return FR_OK;
    }

    fs = dp->fs;
    fno->fname[0] = 0;

    if (fs->fs_type == FS_FAT32) {
        sect = clst2sect(fs, dp->sclust);
    } else {
        sect = fs->dirbase;
    }
    sect += dp->index / (SS(fs) / SZ_DIRE);

    res = move_window(fs, sect);
    if (res != FR_OK) return res;

    for (i = dp->index; i < fs->n_rootdir || (fs->fs_type == FS_FAT32 && i < 512); i++) {
        if (i > dp->index && (i % (SS(fs) / SZ_DIRE)) == 0) {
            sect++;
            res = move_window(fs, sect);
            if (res != FR_OK) return res;
        }

        dir = fs->win + (i % (SS(fs) / SZ_DIRE)) * SZ_DIRE;

        if (dir[DIR_Name] == 0) {
            dp->index = i;
            return FR_OK;
        }
        if (dir[DIR_Name] == DDEM) continue;
        if (dir[DIR_Attr] & 0x08) continue;  /* 跳过卷标 */

        /* 复制文件名 */
        int j, k = 0;
        for (j = 0; j < 8 && dir[j] != ' '; j++) {
            fno->fname[k++] = dir[j];
        }
        if (dir[8] != ' ') {
            fno->fname[k++] = '.';
            for (j = 8; j < 11 && dir[j] != ' '; j++) {
                fno->fname[k++] = dir[j];
            }
        }
        fno->fname[k] = 0;

        fno->fattrib = dir[DIR_Attr];
        fno->fsize = LD_DWORD(&dir[DIR_FileSize]);
        fno->fdate = LD_WORD(&dir[DIR_WrtDate]);
        fno->ftime = LD_WORD(&dir[DIR_WrtTime]);

        dp->index = i + 1;
        return FR_OK;
    }

    dp->index = i;
    return FR_OK;
}

#if !FF_FS_READONLY && FF_USE_MKFS
FRESULT f_mkdir(const TCHAR *path)
{
    /* 简化实现：暂不支持 */
    return FR_NOT_ENABLED;
}

FRESULT f_unlink(const TCHAR *path)
{
    /* 简化实现：暂不支持 */
    return FR_NOT_ENABLED;
}
#endif

FRESULT f_stat(const TCHAR *path, FILINFO *fno)
{
    FRESULT res;
    DIR dj;
    FILINFO fi;

    res = f_opendir(&dj, path);
    if (res != FR_OK) return res;

    /* 简化: 遍历目录查找 */
    const TCHAR *fn = path + 3;
    if (*fn == '/' || *fn == '\\') fn++;

    while ((res = f_readdir(&dj, &fi)) == FR_OK && fi.fname[0]) {
        /* 简化比较 */
        int match = 1;
        int i = 0;
        while (fn[i] && fi.fname[i]) {
            char c1 = fn[i];
            char c2 = fi.fname[i];
            if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
            if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
            if (c1 != c2) { match = 0; break; }
            i++;
        }
        if (match && fn[i] == 0 && fi.fname[i] == 0) {
            if (fno) *fno = fi;
            f_closedir(&dj);
            return FR_OK;
        }
    }

    f_closedir(&dj);
    return FR_NO_FILE;
}

FRESULT f_getfree(const TCHAR *path, DWORD *nclst, FATFS **fatfs)
{
    FRESULT res;
    FATFS *fs;
    DWORD n, clst, sect;
    int vol;

    vol = (int)(path[0] - '0');
    if (vol < 0 || vol >= FF_VOLUMES) return FR_INVALID_DRIVE;

    fs = FatFs[vol];
    if (!fs || !fs->fs_type) return FR_NOT_ENABLED;

    *fatfs = fs;

    if (fs->free_clst <= fs->n_fatent - 2) {
        *nclst = fs->free_clst;
        return FR_OK;
    }

    /* 计算空闲簇 */
    n = 0;
    for (clst = 2; clst < fs->n_fatent; clst++) {
        if (get_fat(fs, clst) == 0) n++;
    }
    fs->free_clst = n;
    *nclst = n;

    return FR_OK;
}

#if FF_USE_STRFUNC
int f_putc(TCHAR c, FIL *fp)
{
    UINT bw;
    BYTE buf[1];

    buf[0] = (BYTE)c;
    if (f_write(fp, buf, 1, &bw) != FR_OK || bw != 1) return -1;
    return c;
}

int f_puts(const TCHAR *str, FIL *fp)
{
    int n = 0;
    while (*str) {
        if (f_putc(*str++, fp) == -1) return -1;
        n++;
    }
    return n;
}

TCHAR *f_gets(TCHAR *buff, int len, FIL *fp)
{
    UINT br;
    int i = 0;
    TCHAR *p = buff;

    while (i < len - 1) {
        if (f_read(fp, p, 1, &br) != FR_OK || br != 1) break;
        if (*p == '\n') { p++; break; }
        if (*p != '\r') { p++; i++; }
    }
    *p = 0;
    return (i > 0) ? buff : NULL;
}
#endif
