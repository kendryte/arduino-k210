/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DBG_TAG "vfs_fat_sd"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"
#include "rtthread.h"

#include <stdlib.h>
#include <string.h>

#include "vfs/esp_vfs.h"
#include "fatfs/esp_vfs_fat.h"

#if FF_MULTI_PARTITION		/* Multiple partition configuration */
const PARTITION VolToPart[FF_VOLUMES] = {
    {0, 0},    /* Logical drive 0 ==> Physical drive 0, auto detection */
#if FF_VOLUMES > 1
    {1, 0},    /* Logical drive 1 ==> Physical drive 1, auto detection */
#endif
#if FF_VOLUMES > 2
    {2, 0},     /* Logical drive 2 ==> Physical drive 2, auto detection */
#endif
#if FF_VOLUMES > 3
    {3, 0},     /* Logical drive 3 ==> Physical drive 3, auto detection */
#endif
#if FF_VOLUMES > 4
    {4, 0},     /* Logical drive 4 ==> Physical drive 4, auto detection */
#endif
#if FF_VOLUMES > 5
    {5, 0},     /* Logical drive 5 ==> Physical drive 5, auto detection */
#endif
#if FF_VOLUMES > 6
    {6, 0},     /* Logical drive 6 ==> Physical drive 6, auto detection */
#endif
#if FF_VOLUMES > 7
    {7, 0},     /* Logical drive 7 ==> Physical drive 7, auto detection */
#endif
#if FF_VOLUMES > 8
    {8, 0},     /* Logical drive 8 ==> Physical drive 8, auto detection */
#endif
#if FF_VOLUMES > 9
    {9, 0},     /* Logical drive 9 ==> Physical drive 9, auto detection */
#endif
};
#endif

static char *s_base_path = NULL;

static esp_err_t format_sdcard(const char *drv, BYTE pdrv)
{
    FRESULT res = FR_OK;
    esp_err_t err;
    const size_t workbuf_size = 4096;
    void* workbuf = NULL;

    LOG_D("formating card");

    workbuf = rt_malloc_align(workbuf_size, 8);
    if (workbuf == NULL) {
        return ESP_ERR_NO_MEM;
    }

#if FF_MULTI_PARTITION
    LBA_t plist[] = {100, 0, 0, 0};
    res = f_fdisk(pdrv, plist, workbuf);
    if (res != FR_OK) {
        err = ESP_FAIL;
        LOG_D("f_fdisk failed (%d)", res);
        goto fail;
    }
#endif

    const MKFS_PARM opt = {(BYTE)FM_ANY | FM_SFD, 0, 0, 0, 0};
    res = f_mkfs(drv, &opt, workbuf, workbuf_size);
    if (res != FR_OK) {
        err = ESP_FAIL;
        LOG_E("f_mkfs failed (%d)", res);
        goto fail;
    }

    rt_free_align(workbuf);

    return ESP_OK;
fail:
    rt_free_align(workbuf);

    return err;
}

esp_err_t esp_vfs_fat_sdspi_mount(const char* base_path, const esp_vfs_fat_mount_config_t* mount_config)
{
    FATFS* fs = NULL;
    esp_err_t err = ESP_OK;

    BYTE pdrv = 0; // only support one drv

    LOG_D("using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    s_base_path = rt_strdup(base_path);
    if(NULL == s_base_path) {
        LOG_E("%s no memory", __func__);
        goto fail;
    }

    // connect FATFS to VFS
    err = esp_vfs_fat_register(s_base_path, drv, mount_config->max_files, &fs);
    if (err == ESP_ERR_INVALID_STATE) {
        // it's okay, already registered with VFS
    } else if (err != ESP_OK) {
        LOG_E("esp_vfs_fat_register failed 0x(%x)", err);
        goto fail;
    }

    // Try to mount partition
    FRESULT res = f_mount(fs, drv, 1);
    if (res != FR_OK) {
        err = ESP_FAIL;
        LOG_W("failed to mount card (%d)", res);
        if (!((res == FR_NO_FILESYSTEM || res == FR_INT_ERR)
              && mount_config->format_if_mount_failed)) {
            goto fail;
        }

        err = format_sdcard(drv, pdrv);
        if (err != ESP_OK) {
            goto fail;
        }

        LOG_W("mounting again");
        res = f_mount(fs, drv, 0);
        if (res != FR_OK) {
            err = ESP_FAIL;
            LOG_W("f_mount failed after formatting (%d)", res);
            goto fail;
        }
    }
    return ESP_OK;

fail:
    esp_vfs_fat_sdspi_unmount();

    return err;
}

esp_err_t esp_vfs_fat_sdspi_format(void)
{
    BYTE pdrv = 0; // only support one drv

    LOG_D("using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    esp_err_t err = format_sdcard(drv, pdrv);

    return err;
}

esp_err_t esp_vfs_fat_sdspi_unmount(void)
{
    BYTE pdrv = 0; // only support one drv

    // unmount
    LOG_D("using pdrv=%i", pdrv);
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    f_unmount(drv);

    esp_err_t err = esp_vfs_fat_unregister_path(s_base_path);

    rt_free(s_base_path);
    s_base_path = NULL;

    return err;
}
