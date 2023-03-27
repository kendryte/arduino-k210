#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// #define CONFIG_FATFS_USE_FASTSEEK                   (0)

#ifdef CONFIG_FATFS_USE_FASTSEEK
#define CONFIG_FATFS_FAST_SEEK_BUFFER_SIZE          (64)
#endif

#ifdef __cplusplus
}
#endif
