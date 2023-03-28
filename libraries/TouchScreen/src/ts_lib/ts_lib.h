#pragma once

#include <limits.h>

#include "rtthread.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define touchscreen_malloc(sz) rt_malloc_align(sz, 8)
#define touchscreen_free(p) rt_free_align(p)

    /**
     * mean.h
     */
    struct mean_filter_t
    {
        int *buffer;
        int length;
        int index;
        int count;
        int sum;
    } __attribute__((aligned(8)));

    struct mean_filter_t *mean_alloc(int length);
    void mean_free(struct mean_filter_t *filter);
    int mean_update(struct mean_filter_t *filter, int value);
    void mean_clear(struct mean_filter_t *filter);

    /**
     * median.h
     */
#ifndef INT_MIN
#define INT_MIN (-1 - 0x7fffffff)
#endif
#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

    struct median_filter_t
    {
        int *buffer;
        int *index;
        int length;
        int position;
        int count;
    } __attribute__((aligned(8)));

    struct median_filter_t *median_alloc(int length);
    void median_free(struct median_filter_t *filter);
    int median_update(struct median_filter_t *filter, int value);
    void median_clear(struct median_filter_t *filter);

    /**
     * tsfilter.h
     */
    struct tsfilter_t
    {
        struct median_filter_t *mx, *my;
        struct mean_filter_t *nx, *ny;
        int cal[7];
    } __attribute__((aligned(8)));

    struct tsfilter_t *tsfilter_alloc(int ml, int nl);
    void tsfilter_free(struct tsfilter_t *filter);
    void tsfilter_setcal(struct tsfilter_t *filter, int *cal);
    void tsfilter_update(struct tsfilter_t *filter, int *x, int *y);
    void tsfilter_clear(struct tsfilter_t *filter);

    /**
     * event.h
     */

    enum event_type
    {
        TOUCH_NONE = 0,
        TOUCH_BEGIN,
        TOUCH_MOVE,
        TOUCH_END
    };

    struct ts_event_t
    {
        enum event_type type;
        int x, y;
    } __attribute__((aligned(8)));

    void push_event_begin(struct ts_event_t *ts_event, int x, int y);
    void push_event_move(struct ts_event_t *ts_event, int x, int y);
    void push_event_end(struct ts_event_t *ts_event, int x, int y);
    void push_event_none(struct ts_event_t *ts_event);

    /**
     * tscal.h
     */
    struct tscal_t
    {
        int x[5], xfb[5];
        int y[5], yfb[5];
        int a[7];
    } __attribute__((aligned(8)));

    int perform_calibration(struct tscal_t *cal);

    enum ts_ioctl_type
    {
        TS_IOCTL_SET_CALBRATION = 1,
    };

    struct ts_pdata_t
    {
        struct tsfilter_t *filter;
        struct ts_event_t *event;
        int x, y;
        int press;
    } __attribute__((aligned(8)));

#ifdef __cplusplus
}
#endif
