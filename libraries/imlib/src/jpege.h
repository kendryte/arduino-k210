#pragma once

#include "imlib.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OMV_JPEG_CODEC_ENABLE (0)

////////////////
// JPEG Stuff //
////////////////

#define JPEG_MCU_W (8)
#define JPEG_MCU_H (8)
#define JPEG_444_GS_MCU_SIZE ((JPEG_MCU_W) * (JPEG_MCU_H))
#define JPEG_444_YCBCR_MCU_SIZE ((JPEG_444_GS_MCU_SIZE) * 3)
#define JPEG_422_YCBCR_MCU_SIZE ((JPEG_444_GS_MCU_SIZE) * 4)
#define JPEG_420_YCBCR_MCU_SIZE ((JPEG_444_GS_MCU_SIZE) * 6)

typedef enum jpeg_subsampling {
  JPEG_SUBSAMPLING_AUTO = 0,
  JPEG_SUBSAMPLING_444 = 0x11, // Chroma subsampling 4:4:4 (No subsampling)
  JPEG_SUBSAMPLING_422 = 0x21, // Chroma subsampling 4:2:2
  JPEG_SUBSAMPLING_420 = 0x22, // Chroma subsampling 4:2:0
} jpeg_subsampling_t;

int jpeg_compress(image_t *src, uint8_t *dst_data, size_t *dst_size, int quality, jpeg_subsampling_t subsampling);

#ifdef __cplusplus
}
#endif
