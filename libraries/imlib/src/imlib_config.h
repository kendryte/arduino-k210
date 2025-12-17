#pragma once

// Attempt to include the header file
#ifdef __has_include
#if __has_include("imlib_config_user.h")
#include "imlib_config_user.h"
#endif
#endif

#undef OMV_JPEG_CODEC_ENABLE
#define OMV_JPEG_CODEC_ENABLE (0)

// enable Image read and write
// #define IMLIB_ENABLE_IMAGE_FILE_IO (0)

// enable imlib_histeq
// #define IMLIB_ENABLE_HISTEQ

// enable imlib_mean_filter
// #define IMLIB_ENABLE_MEAN

// enable imlib_median_filter
// #define IMLIB_ENABLE_MEDIAN

// enable imlib_mode_filter
// #define IMLIB_ENABLE_MODE

// enable imlib_midpoint_filter
// #define IMLIB_ENABLE_MIDPOINT

// enable imlib_morph
// #define IMLIB_ENABLE_MORPH

// enable imlib_bilateral_filter
// #define IMLIB_ENABLE_BILATERAL
