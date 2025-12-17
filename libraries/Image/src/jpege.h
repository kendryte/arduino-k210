#pragma once

#include <stddef.h>
#include <stdint.h>

#include "imlib.h"

#ifdef __cplusplus
extern "C" {
#endif

int jpeg_compress(ImageInfo_t *src, uint8_t *dst_data, size_t *dst_size,
                  int quality, jpeg_subsampling_t subsampling);

#ifdef __cplusplus
}
#endif
