#ifndef _YOLO2_REGION_LAYER
#define _YOLO2_REGION_LAYER

#include <stdint.h>
#include <math.h>

#include "kpu_helper.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define REGION_LAYER_MAX_OBJ_NUM (10)

    typedef struct
    {
        uint32_t obj_number;

        struct
        {
            uint32_t x;
            uint32_t y;
            uint32_t w;
            uint32_t h;
            uint32_t class_id;
            float prob;
        } obj[REGION_LAYER_MAX_OBJ_NUM];
    } obj_info_t;

    typedef struct
    {
        float threshold;
        float nms_value;
        uint32_t coords;
        uint32_t anchor_number;
        float *anchor;
        uint32_t image_width;
        uint32_t image_height;
        uint32_t classes;
        uint32_t net_width;
        uint32_t net_height;
        uint32_t layer_width;
        uint32_t layer_height;
        uint32_t boxes_number;
        uint32_t output_number;
        void *boxes;
        float *input;
        float *output;
        float *probs_buf;
        float **probs;
    } yolo2_region_layer_t;

    int yolo_region_layer_init(yolo2_region_layer_t *rl, k210_kpu_shape_t *input, k210_kpu_shape_t *output);
    void yolo_region_layer_deinit(yolo2_region_layer_t *rl);
    void region_layer_run(yolo2_region_layer_t *rl, obj_info_t *obj_info);

#ifdef __cplusplus
}
#endif

#endif // _YOLO2_REGION_LAYER
