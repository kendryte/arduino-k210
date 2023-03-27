#define DBG_TAG "KPU_Face"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtthread.h"

#include "KPU_Face.h"

#include <float.h>
#include <limits.h>
#include <math.h>

namespace K210
{

    static void svd22(const float a[4], float u[4], float s[2], float v[4])
    {
        s[0] = (sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)) + sqrt(pow(a[0] + a[3], 2) + pow(a[1] - a[2], 2))) / 2;
        s[1] = fabs(s[0] - sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)));
        v[2] = (s[0] > s[1]) ? sin((atan2(2 * (a[0] * a[1] + a[2] * a[3]), a[0] * a[0] - a[1] * a[1] + a[2] * a[2] - a[3] * a[3])) / 2) : 0;
        v[0] = sqrt(1 - v[2] * v[2]);
        v[1] = -v[2];
        v[3] = v[0];
        u[0] = (s[0] != 0) ? -(a[0] * v[0] + a[1] * v[2]) / s[0] : 1;
        u[2] = (s[0] != 0) ? -(a[2] * v[0] + a[3] * v[2]) / s[0] : 0;
        u[1] = (s[1] != 0) ? (a[0] * v[1] + a[1] * v[3]) / s[1] : -u[2];
        u[3] = (s[1] != 0) ? (a[2] * v[1] + a[3] * v[3]) / s[1] : u[0];
        v[0] = -v[0];
        v[2] = -v[2];
    }

    static void l2normalize(float *x, float *dx, int len)
    {
        int f;
        float sum = 0;
        for (f = 0; f < len; ++f)
        {
            sum += x[f] * x[f];
        }
        sum = sqrtf(sum);
        for (f = 0; f < len; ++f)
        {
            dx[f] = x[f] / sum;
        }
    }

    static float calCosinDistance(float *faceFeature0P, float *faceFeature1P, int featureLen)
    {
        float coorFeature = 0;

        for (int fIdx = 0; fIdx < featureLen; fIdx++)
        {
            float featureVal0 = *(faceFeature0P + fIdx);
            float featureVal1 = *(faceFeature1P + fIdx);
            coorFeature += featureVal0 * featureVal1;
        }

        return (0.5 + 0.5 * coorFeature) * 100;
    }

    KPU_Face::KPU_Face() : KPU_Base()
    {

    }

    KPU_Face::~KPU_Face()
    {

    }

    int KPU_Face::calc_affine_transform(uint16_t *src, uint16_t *dst, uint16_t cnt, float *TT)
    {
    #define MAX_POINT_CNT 10

        int i, j, k;
        float src_mean[2] = {0.0f};
        float dst_mean[2] = {0.0f};

        if(MAX_POINT_CNT < cnt)
        {
            LOG_E("affine cnt too large");
            return -1;
        }

        for (i = 0; i < cnt * 2; i += 2)
        {
            src_mean[0] += dst[i];
            src_mean[1] += dst[i + 1];
            dst_mean[0] += src[i];
            dst_mean[1] += src[i + 1];
        }
        src_mean[0] /= cnt;
        src_mean[1] /= cnt;
        dst_mean[0] /= cnt;
        dst_mean[1] /= cnt;

        float src_demean[MAX_POINT_CNT][2] = {0.0f};
        float dst_demean[MAX_POINT_CNT][2] = {0.0f};

        for (i = 0; i < cnt; i++)
        {
            src_demean[i][0] = dst[2 * i] - src_mean[0];
            src_demean[i][1] = dst[2 * i + 1] - src_mean[1];
            dst_demean[i][0] = src[2 * i] - dst_mean[0];
            dst_demean[i][1] = src[2 * i + 1] - dst_mean[1];
        }

        float A[2][2] = {0.0f};
        for (i = 0; i < 2; i++)
        {
            for (k = 0; k < 2; k++)
            {
                for (j = 0; j < cnt; j++)
                {
                    A[i][k] += dst_demean[j][i] * src_demean[j][k];
                }
                A[i][k] /= cnt;
            }
        }

        float(*T)[3] = (float(*)[3])TT;
        T[0][0] = 1; T[0][1] = 0; T[0][2] = 0;
        T[1][0] = 0; T[1][1] = 1; T[1][2] = 0;
        T[2][0] = 0; T[2][1] = 0; T[2][2] = 1;

        float U[2][2] = {0};
        float S[2] = {0};
        float V[2][2] = {0};
        svd22(&A[0][0], &U[0][0], S, &V[0][0]);

        T[0][0] = U[0][0] * V[0][0] + U[0][1] * V[1][0];
        T[0][1] = U[0][0] * V[0][1] + U[0][1] * V[1][1];
        T[1][0] = U[1][0] * V[0][0] + U[1][1] * V[1][0];
        T[1][1] = U[1][0] * V[0][1] + U[1][1] * V[1][1];

        float scale = 1.0f;
        float src_demean_mean[2] = {0.0f};
        float src_demean_var[2] = {0.0f};
        for (i = 0; i < cnt; i++)
        {
            src_demean_mean[0] += src_demean[i][0];
            src_demean_mean[1] += src_demean[i][1];
        }
        src_demean_mean[0] /= cnt;
        src_demean_mean[1] /= cnt;

        for (i = 0; i < cnt; i++)
        {
            src_demean_var[0] += (src_demean_mean[0] - src_demean[i][0]) * (src_demean_mean[0] - src_demean[i][0]);
            src_demean_var[1] += (src_demean_mean[1] - src_demean[i][1]) * (src_demean_mean[1] - src_demean[i][1]);
        }
        src_demean_var[0] /= (cnt);
        src_demean_var[1] /= (cnt);
        scale = 1.0f / (src_demean_var[0] + src_demean_var[1]) * (S[0] + S[1]);
        T[0][2] = dst_mean[0] - scale * (T[0][0] * src_mean[0] + T[0][1] * src_mean[1]);
        T[1][2] = dst_mean[1] - scale * (T[1][0] * src_mean[0] + T[1][1] * src_mean[1]);
        T[0][0] *= scale;
        T[0][1] *= scale;
        T[1][0] *= scale;
        T[1][1] *= scale;

        return 0;

    #undef MAX_POINT_CNT
    }

    int KPU_Face::apply_affine_transform(Image *src, Image *dst, float *TT)
    {
        if ((IMAGE_FORMAT_R8G8B8 != src->format) || (IMAGE_FORMAT_R8G8B8 != dst->format))
        {
            LOG_E("only support r8g8b8");
            return -1;
        }

        int i, j, k;
        int step = src->w;
        int color_step = src->w * src->h;

        uint8_t *src_buf = src->pixel;
        uint8_t *dst_buf = dst->pixel;

        if (src_buf == NULL || dst_buf == NULL)
        {
            return -2;
        }

        int dst_color_step = dst->w * dst->h;
        int dst_step = dst->w;

        memset(dst_buf, 0, dst->w * dst->h * dst->bpp);

        int x, y, pre_x, pre_y; 
        unsigned short color[2][2];
        float(*T)[3] = (float(*)[3])TT;

        for (i = 0; i < dst->h; i++)
        {
            for (j = 0; j < dst->w; j++)
            {
                pre_x = (int)(T[0][0] * (j << 8) + T[0][1] * (i << 8) + T[0][2] * (1 << 8));
                pre_y = (int)(T[1][0] * (j << 8) + T[1][1] * (i << 8) + T[1][2] * (1 << 8));

                y = pre_y & 0xFF;
                x = pre_x & 0xFF;

                pre_x >>= 8;
                pre_y >>= 8;

                if (pre_x < 0 || pre_x > (src->w - 1) || pre_y < 0 || pre_y > (src->h - 1))
                    continue;

                for (k = 0; k < src->bpp; k++)
                {
                    color[0][0] = src_buf[pre_y * step + pre_x + k * color_step];
                    color[1][0] = src_buf[pre_y * step + (pre_x + 1) + k * color_step];
                    color[0][1] = src_buf[(pre_y + 1) * step + pre_x + k * color_step];
                    color[1][1] = src_buf[(pre_y + 1) * step + (pre_x + 1) + k * color_step];
                    int final = (256 - x) * (256 - y) * color[0][0] + x * (256 - y) * color[1][0] + (256 - x) * y * color[0][1] + x * y * color[1][1];
                    final = final >> 16;
                    dst_buf[i * dst_step + j + k * dst_color_step] = final;
                }
            }
        }

        return 0;
    }

    int KPU_Face::calc_feature(float *feature)
    {
        float *result;
        size_t output_size;

        if(0x00 != get_result((uint8_t **)&result, &output_size))
        {
            LOG_E("get result failed");
            return -1;
        }

        l2normalize(result, feature, output_size / sizeof(float));

        return 0;
    }

    float KPU_Face::compare_feature(float *f1, float *f2, size_t length)
    {
        return calCosinDistance(f1, f2, length);
    }

}
