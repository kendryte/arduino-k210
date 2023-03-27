#define DBG_TAG "DRV_RTC"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "rtdef.h"
#include "rtthread.h"

#ifdef RT_USING_RTC
#include "rtdevice.h"

#include "k210-hal.h"

static rt_err_t drv_rtc_init(void)
{
    rtc_init();
    rtc_timer_set(1970, 1, 1, 8, 0, 0); //1970-01-01 08:00:00

    return RT_EOK;
}

static rt_err_t drv_rtc_get_secs(time_t *sec)
{
    struct tm * tm = rtc_timer_get_tm();
    time_t t = mktime(tm);
    *sec = t;

    return RT_EOK;
}

static rt_err_t drv_rtc_set_secs(time_t *sec)
{
    struct tm * tm = gmtime(sec);

    return rtc_timer_set_tm(tm);
}

// static rt_err_t drv_rtc_get_alarm(struct rt_rtc_wkalarm *alarm)
// {
// }

// static rt_err_t drv_rtc_set_alarm(struct rt_rtc_wkalarm *alarm)
// {
// }

// static rt_err_t drv_rtc_get_timeval(struct timeval *tv)
// {
// }

// static rt_err_t drv_rtc_set_timeval(struct timeval *tv)
// {
// }

static const struct rt_rtc_ops k210_rtc_ops = {
    drv_rtc_init,
    drv_rtc_get_secs,
    drv_rtc_set_secs,
    NULL, //drv_rtc_get_alarm,
    NULL, //drv_rtc_set_alarm,
    NULL, //drv_rtc_get_timeval,
    NULL, //drv_rtc_set_timeval,
};

static rt_rtc_dev_t k210_rtc_dev = {
    .ops = &k210_rtc_ops,
};

int initRtc(void)
{
    rt_err_t result = rt_hw_rtc_register(&k210_rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK) {
        LOG_E("rtc register err code: %d", result);
        return result;
    }
    rt_kprintf("rtc init success\r\n");

    return 0;
}

#endif /* RT_USING_RTC */
