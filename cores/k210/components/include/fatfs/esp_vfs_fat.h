/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include "espressif/esp_err.h"

#include "fatfs/fatfs/ff.h"

#include "fatfs/esp_vfs_fat_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register FATFS with VFS component
 *
 * This function registers given FAT drive in VFS, at the specified base path.
 * If only one drive is used, fat_drive argument can be an empty string.
 * Refer to FATFS library documentation on how to specify FAT drive.
 * This function also allocates FATFS structure which should be used for f_mount
 * call.
 *
 * @note This function doesn't mount the drive into FATFS, it just connects
 *       POSIX and C standard library IO function with FATFS. You need to mount
 *       desired drive into FATFS separately.
 *
 * @param base_path  path prefix where FATFS should be registered
 * @param fat_drive  FATFS drive specification; if only one drive is used, can be an empty string
 * @param max_files  maximum number of files which can be open at the same time
 * @param[out] out_fs  pointer to FATFS structure which can be used for FATFS f_mount call is returned via this argument.
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if esp_vfs_fat_register was already called
 *      - ESP_ERR_NO_MEM if not enough memory or too many VFSes already registered
 */
esp_err_t esp_vfs_fat_register(const char* base_path, const char* fat_drive,
        size_t max_files, FATFS** out_fs);

/**
 * @brief Un-register FATFS from VFS
 *
 * @note FATFS structure returned by esp_vfs_fat_register is destroyed after
 *       this call. Make sure to call f_mount function to unmount it before
 *       calling esp_vfs_fat_unregister_ctx.
 *       Difference between this function and the one above is that this one
 *       will release the correct drive, while the one above will release
 *       the last registered one
 *
 * @param base_path     path prefix where FATFS is registered. This is the same
 *                      used when esp_vfs_fat_register was called
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if FATFS is not registered in VFS
 */
esp_err_t esp_vfs_fat_unregister_path(const char* base_path);

/**
 * @brief Configuration arguments for esp_vfs_fat_sdmmc_mount and esp_vfs_fat_spiflash_mount_rw_wl functions
 */
typedef struct {
    /**
     * If FAT partition can not be mounted, and this parameter is true,
     * create partition table and format the filesystem.
     */
    bool format_if_mount_failed;
    int max_files;                  ///< Max number of open files
    /**
     * If format_if_mount_failed is set, and mount fails, format the card
     * with given allocation unit size. Must be a power of 2, between sector
     * size and 128 * sector size.
     * For SD cards, sector size is always 512 bytes. For wear_levelling,
     * sector size is determined by CONFIG_WL_SECTOR_SIZE option.
     *
     * Using larger allocation unit size will result in higher read/write
     * performance and higher overhead when storing small files.
     *
     * Setting this field to 0 will result in allocation unit set to the
     * sector size.
     */
    size_t allocation_unit_size;
    /**
     * Enables real ff_disk_status function implementation for SD cards
     * (ff_sdmmc_status). Possibly slows down IO performance.
     *
     * Try to enable if you need to handle situations when SD cards
     * are not unmounted properly before physical removal
     * or you are experiencing issues with SD cards.
     *
     * Doesn't do anything for other memory storage media.
     */
    bool disk_status_check_enable;
} esp_vfs_fat_mount_config_t;

esp_err_t esp_vfs_fat_sdspi_mount(const char* base_path, const esp_vfs_fat_mount_config_t* mount_config);
esp_err_t esp_vfs_fat_sdspi_format(void);
esp_err_t esp_vfs_fat_sdspi_unmount(void);

/**
 * @brief  Get information for FATFS partition
 *
 * @param base_path  Path where partition should be registered (e.g. "/spiflash")
 * @param[out] out_total_bytes  Size of the file system
 * @param[out] out_free_bytes   Current used bytes in the file system
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if partition not found
 *      - ESP_FAIL if another FRESULT error (saved in errno)
 */
esp_err_t esp_vfs_fat_info(const char* base_path, uint64_t* out_total_bytes, uint64_t* out_free_bytes);

#ifdef __cplusplus
}
#endif
