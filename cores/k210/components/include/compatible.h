#pragma once

#include <stdint.h>
#include <stddef.h>

#include "espressif/esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3)) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#ifndef likely
#define likely(expr)     expect((expr) != 0, 1)
#endif
#ifndef unlikely
#define unlikely(expr)   expect((expr) != 0, 0)
#endif

/**
 * @brief partition information structure
 *
 * This is not the format in flash, that format is esp_partition_info_t.
 *
 * However, this is the format used by this API.
 */
typedef struct {
    // esp_flash_t* flash_chip;            /*!< SPI flash chip on which the partition resides */
    // esp_partition_type_t type;          /*!< partition type (app/data) */
    // esp_partition_subtype_t subtype;    /*!< partition subtype */
    uint32_t address;                   /*!< starting address of the partition in flash */
    uint32_t size;                      /*!< size of the partition, in bytes */
    // uint32_t erase_size;                /*!< size the erase operation should be aligned to */
    char label[17];                     /*!< partition label, zero-terminated ASCII string */
    // bool encrypted;                     /*!< flag is set to true if partition is encrypted */
} esp_partition_t;


const esp_partition_t *esp_partition_find_first(const char *label);

esp_err_t esp_partition_erase_range(const esp_partition_t* partition, size_t offset, size_t size);

size_t strlcat(char * dest, const char * src, size_t n);
size_t strlcpy(char * dest, const char * src, size_t n);

#ifdef __cplusplus
} // extern "C"
#endif
