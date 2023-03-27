#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define maximum number of partitions that can be mounted.
*/
#define CONFIG_SPIFFS_MAX_PARTITIONS            3

/**
 * Enables/disable memory read caching of nucleus file system operations.
*/
#define CONFIG_SPIFFS_CACHE                     1

/**
 * Enables memory write caching for file descriptors in hydrogen.
*/
#define CONFIG_SPIFFS_CACHE_WR                  1

/**
 * Enable/disable statistics on caching. Debug/test purpose only.
*/
// #define CONFIG_SPIFFS_CACHE_STATS            0

/**
 * Always check header of each accessed page to ensure consistent state.
 * If enabled it will increase number of reads from flash, especially
 * if cache is disabled.
*/ 
#define CONFIG_SPIFFS_PAGE_CHECK                1

/**
 * Define maximum number of GC runs to perform to reach desired free pages.
*/
#define CONFIG_SPIFFS_GC_MAX_RUNS               10

/**
 * Enable/disable statistics on gc. Debug/test purpose only.
*/
// #define CONFIG_SPIFFS_GC_STATS               0

/**
 * Logical page size of SPIFFS partition, in bytes. Must be multiple
 * of flash page size (which is usually 256 bytes).
 * Larger page sizes reduce overhead when storing large files, and
 * improve filesystem performance when reading large files.
 * Smaller page sizes reduce overhead when storing small (< page size)
 * files.
*/
#define CONFIG_SPIFFS_PAGE_SIZE                 256

/**
 * Object name maximum length. Note that this length include the
 * zero-termination character, meaning maximum string of characters
 * can at most be SPIFFS_OBJ_NAME_LEN - 1.
 * 
 * SPIFFS_OBJ_NAME_LEN + SPIFFS_META_LENGTH should not exceed
 * SPIFFS_PAGE_SIZE - 64.
*/
#define CONFIG_SPIFFS_OBJ_NAME_LEN              32

/**
 * If this option is enabled, symbolic links are taken into account
 * during partition image creation.
*/
#define CONFIG_SPIFFS_FOLLOW_SYMLINKS           1

/**
 * Enable this to have an identifiable spiffs filesystem.
 * This will look for a magic in all sectors to determine if this
 * is a valid spiffs system or not at mount time.
*/
#define CONFIG_SPIFFS_USE_MAGIC                 1

#ifdef CONFIG_SPIFFS_USE_MAGIC
/**
 * If this option is enabled, the magic will also be dependent
 * on the length of the filesystem. For example, a filesystem
 * configured and formatted for 4 megabytes will not be accepted
 * for mounting with a configuration defining the filesystem as 2 megabytes.
*/
#define CONFIG_SPIFFS_USE_MAGIC_LENGTH          1

#endif // CONFIG_SPIFFS_USE_MAGIC

/**
 * This option sets the number of extra bytes stored in the file header.
 * These bytes can be used in an application-specific manner.
 * Set this to at least 4 bytes to enable support for saving file
 * modification time.
 * 
 * SPIFFS_OBJ_NAME_LEN + SPIFFS_META_LENGTH should not exceed
 * SPIFFS_PAGE_SIZE - 64.
*/
#define CONFIG_SPIFFS_META_LENGTH               8

#if CONFIG_SPIFFS_META_LENGTH >= 4
/**
 * If enabled, then the first 4 bytes of per-file metadata will be used
 * to store file modification time (mtime), accessible through
 * stat/fstat functions.
 * Modification time is updated when the file is opened.
*/
#define CONFIG_SPIFFS_USE_MTIME                 1

#endif // CONFIG_SPIFFS_META_LENGTH >= 4

#if CONFIG_SPIFFS_META_LENGTH >= 8
/**
 * If this option is not set, the time field is 32 bits (up to 2106 year),
 * otherwise it is 64 bits and make sure it matches SPIFFS_META_LENGTH.
 * If the chip already has the spiffs image with the time field = 32 bits
 * then this option cannot be applied in this case.
 * Erase it first before using this option.
 * To resolve the Y2K38 problem for the spiffs, use a toolchain with
 * 64-bit time_t support.
*/
#define CONFIG_SPIFFS_MTIME_WIDE_64_BITS        1

#endif // CONFIG_SPIFFS_META_LENGTH >= 8

/**
 * Enabling this option will print general debug mesages to the console.
*/
// #define CONFIG_SPIFFS_DBG                       1

/**
 * Enabling this option will print API debug mesages to the console.
*/
// #define CONFIG_SPIFFS_API_DBG                   1

/**
 * Enabling this option will print GC debug mesages to the console.
*/
// #define CONFIG_SPIFFS_GC_DBG                    1

/**
 * Enabling this option will print cache debug mesages to the console.
*/
// #define CONFIG_SPIFFS_CACHE_DBG                 1

/**
 * Enabling this option will print Filesystem Check debug mesages to the console.
*/
// #define CONFIG_SPIFFS_CHECK_DBG                 1

/**
 * Enable this option to enable SPIFFS_vis function in the API.
*/
// #define CONFIG_SPIFFS_TEST_VISUALISATION     0

#ifdef __cplusplus
} // extern "C"
#endif
