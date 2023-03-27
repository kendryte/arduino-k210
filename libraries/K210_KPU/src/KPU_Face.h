#pragma once

#include "KPU.h"

namespace K210
{
    class KPU_Face : public KPU_Base
    {
    public:
        KPU_Face();
        ~KPU_Face();

        int calc_feature(float *feature);
        static float compare_feature(float *f1, float *f2, size_t length);

        static int calc_affine_transform(uint16_t *src, uint16_t *dst, uint16_t cnt, float* TT);
        static int apply_affine_transform(Image * src, Image* dst, float* TT);
    };

}
