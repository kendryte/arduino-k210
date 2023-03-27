/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DBG_TAG "SPIFFS_API"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "./spiffs_api.h"

#include "k210-hal.h"

void spiffs_api_lock(spiffs *fs)
{
    // (void) xSemaphoreTake(((esp_spiffs_t *)(fs->user_data))->lock, portMAX_DELAY);

    esp_spiffs_t *spiffs = (esp_spiffs_t *)fs->user_data;
    rt_mutex_take(&spiffs->lock, RT_WAITING_FOREVER);
}

void spiffs_api_unlock(spiffs *fs)
{
    // xSemaphoreGive(((esp_spiffs_t *)(fs->user_data))->lock);

    esp_spiffs_t *spiffs = (esp_spiffs_t *)fs->user_data;
    rt_mutex_release(&spiffs->lock);
}

void spiffs_api_init(void)
{
    // hal_flash_init(25 * 1000 * 1000, NULL);

    // hal_flash_set_quad_mode();
}

s32_t spiffs_api_read(spiffs *fs, uint32_t addr, uint32_t size, uint8_t *dst)
{
    // esp_err_t err = esp_partition_read(((esp_spiffs_t *)(fs->user_data))->partition, addr, dst, size);

    esp_spiffs_t *spiffs = (esp_spiffs_t *)fs->user_data;
    uint32_t read_addr = spiffs->partition->address + addr;

    flash_status_t err = hal_flash_read_data(read_addr, dst, size);

    if (unlikely(err)) {
        LOG_E("failed to read addr 0x%08" PRIx32 ", size 0x%08" PRIx32 ", err %d", addr, size, err);
        return -1;
    }

    return 0;
}

s32_t spiffs_api_write(spiffs *fs, uint32_t addr, uint32_t size, uint8_t *src)
{
    // esp_err_t err = esp_partition_write(((esp_spiffs_t *)(fs->user_data))->partition, addr, src, size);

    esp_spiffs_t *spiffs = (esp_spiffs_t *)fs->user_data;
    uint32_t write_addr = spiffs->partition->address + addr;

    flash_status_t err = hal_flash_write_data(write_addr, src, size);

    if (unlikely(err)) {
        LOG_E("failed to write addr 0x%08" PRIx32 ", size 0x%08" PRIx32 ", err %d", addr, size, err);
        return -1;
    }

    return 0;
}

s32_t spiffs_api_erase(spiffs *fs, uint32_t addr, uint32_t size)
{
    // esp_err_t err = esp_partition_erase_range(((esp_spiffs_t *)(fs->user_data))->partition, addr, size);

    esp_spiffs_t *spiffs = (esp_spiffs_t *)fs->user_data;
    flash_status_t err = hal_flash_erase(spiffs->partition->address + addr, size);

    if (err) {
        LOG_E("failed to erase addr 0x%08" PRIx32 ", size 0x%08" PRIx32 ", err %d", addr, size, err);
        return -1;
    }

    return 0;
}

void spiffs_api_check(spiffs *fs, spiffs_check_type type, spiffs_check_report report, uint32_t arg1, uint32_t arg2)
{
    static const char * spiffs_check_type_str[3] = {
        "LOOKUP",
        "INDEX",
        "PAGE"
    };

    static const char * spiffs_check_report_str[7] = {
        "PROGRESS",
        "ERROR",
        "FIX INDEX",
        "FIX LOOKUP",
        "DELETE ORPHANED INDEX",
        "DELETE PAGE",
        "DELETE BAD FILE"
    };

    if (report != SPIFFS_CHECK_PROGRESS) {
        LOG_E("CHECK: type:%s, report:%s, %" PRIx32 ":%" PRIx32, spiffs_check_type_str[type], spiffs_check_report_str[report], arg1, arg2);
    } else {
        LOG_I("CHECK PROGRESS: report:%s, %" PRIx32 ":%" PRIx32, spiffs_check_report_str[report], arg1, arg2);
    }
}
