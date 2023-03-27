// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define DBG_TAG "FFat"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"
#include "rtthread.h"

#include "vfs_api.h"

extern "C" {
#include "fatfs/esp_vfs_fat.h"
}
#include "FFat.h"

using namespace fs;

#define CONFIG_WL_SECTOR_SIZE 512

F_Fat::F_Fat(FSImplPtr impl)
    : FS(impl), _inited(false)
{}

bool F_Fat::begin(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles)
{
    if(_inited != false){
        LOG_W("Already Mounted!");
        return true;
    }

    esp_vfs_fat_mount_config_t conf = {
      .format_if_mount_failed = formatOnFail,
      .max_files = maxOpenFiles,
      .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_err_t err = esp_vfs_fat_sdspi_mount(basePath, &conf);
    if(err){
        LOG_E("Mounting FFat partition failed! Error: %d", err);
        esp_vfs_fat_sdspi_unmount();
        return false;
    }
    _inited = true;
    _impl->mountpoint(basePath);
    return true;
}

void F_Fat::end()
{
    if(_inited != false){
        esp_err_t err = esp_vfs_fat_sdspi_unmount();
        if(err){
            LOG_E("Unmounting FFat partition failed! Error: %d", err);
            return;
        }
        _inited = false;
        _impl->mountpoint(NULL);
    }
}

bool F_Fat::format(void)
{
    esp_err_t err = ESP_OK;
    bool res = true;

    if(false != _inited){
        LOG_W("Already Mounted!");
        return false;
    }

    err = esp_vfs_fat_sdspi_format();
    if(ESP_OK != err) {
        res = false;
        LOG_E("format sdcard failed!");
        return res;
    }
   
    esp_vfs_fat_mount_config_t conf = {
      .format_if_mount_failed = true,
      .max_files = 1,
      .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    err  = esp_vfs_fat_sdspi_mount("/format_ffat", &conf);
    err += esp_vfs_fat_sdspi_unmount();
    if (err != ESP_OK) {
        res = false;
        LOG_W("esp_vfs_fat_spiflash_mount failed!");
    }

    return res;
}

size_t F_Fat::totalBytes()
{
    FATFS *fs;
    DWORD free_clust, tot_sect, sect_size;

    BYTE pdrv = 0;//ff_diskio_get_pdrv_wl(_wl_handle);
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    if ( f_getfree(drv, &free_clust, &fs) != FR_OK){
        return 0;
    }
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    sect_size = CONFIG_WL_SECTOR_SIZE;
    return (size_t)((size_t)tot_sect * (size_t)sect_size);
}

size_t F_Fat::usedBytes()
{
    FATFS *fs;
    DWORD free_clust, used_sect, sect_size;

    BYTE pdrv = 0;//ff_diskio_get_pdrv_wl(_wl_handle);
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    if ( f_getfree(drv, &free_clust, &fs) != FR_OK){
        return 0;
    }
    used_sect = (fs->n_fatent - 2 - free_clust) * fs->csize;
    sect_size = CONFIG_WL_SECTOR_SIZE;
    return (size_t)((size_t)used_sect * (size_t)sect_size);
}

size_t F_Fat::freeBytes()
{
    FATFS *fs;
    DWORD free_clust, free_sect, sect_size;

    BYTE pdrv = 0;//ff_diskio_get_pdrv_wl(_wl_handle);
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    if ( f_getfree(drv, &free_clust, &fs) != FR_OK){
        return 0;
    }
    free_sect = free_clust * fs->csize;
    sect_size = CONFIG_WL_SECTOR_SIZE;
    return (size_t)((size_t)free_sect * (size_t)sect_size);
}

bool F_Fat::exists(const char* path)
{
    File f = open(path, "r",false);
    return (f == true) && !f.isDirectory();
}

bool F_Fat::exists(const String& path)
{
    return exists(path.c_str());
}

F_Fat FFat = F_Fat(FSImplPtr(new VFSImpl()));
