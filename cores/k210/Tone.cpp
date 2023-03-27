#define DBG_TAG "Tone"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include <Arduino.h>

#include "k210-hal.h"

#include "rtthread.h"

static rt_thread_t _tone_task = NULL;
static rt_mq_t _tone_queue = NULL;

typedef enum {
    TONE_START,
    TONE_END,
} tone_cmd_t;

typedef struct {
    tone_cmd_t tone_cmd;
    uint8_t pin;
    unsigned int frequency;
    unsigned long duration;
} tone_msg_t;

static void tone_task(void *arg)
{
    tone_msg_t tone_msg;

    while (1) {
        if(RT_EOK != rt_mq_recv(_tone_queue, &tone_msg, sizeof(tone_msg_t), RT_WAITING_FOREVER)) {
            continue;
        }

        switch (tone_msg.tone_cmd) {
            case TONE_START: {
                LOG_D("Task received from queue TONE_START: _pin=%d, frequency=%u Hz, duration=%lu ms", tone_msg.pin, tone_msg.frequency, tone_msg.duration);

                struct timer_dev_chn_map_t *info = hal_timer_set_pin_func(tone_msg.pin);

                if(NULL != info) {
                    pwm_init(info->dev.pwm);
                    pwm_set_frequency(info->dev.pwm, info->chn.pwm, tone_msg.frequency, 0.5);
                    pwm_set_enable(info->dev.pwm, info->chn.pwm, 1);

                    if (tone_msg.duration) {
                        rt_thread_mdelay(tone_msg.duration);

                        pwm_set_frequency(info->dev.pwm, info->chn.pwm, tone_msg.frequency, 0);
                        pwm_set_enable(info->dev.pwm, info->chn.pwm, 0);

                        hal_timer_clr_pin_func(tone_msg.pin);
                    }
                }
                break;
            } case TONE_END: {
                LOG_D("Task received from queue TONE_END: pin=%d", tone_msg.pin);

                struct timer_dev_chn_map_t *info = hal_timer_set_pin_func(tone_msg.pin);

                if(NULL != info) {
                    pwm_set_frequency(info->dev.pwm, info->chn.pwm, tone_msg.frequency, 0);
                    pwm_set_enable(info->dev.pwm, info->chn.pwm, 0);

                    hal_timer_clr_pin_func(tone_msg.pin);
                }
                break;
            } default:; // do nothing
        }               // switch
    }                   // infinite loop
}

int tone_init(void)
{
    if (_tone_queue == NULL) {

        LOG_I("Creating tone queue");

        _tone_queue = rt_mq_create("ToneMSG", sizeof(tone_msg_t), 12, RT_IPC_FLAG_FIFO);

        if (_tone_queue == NULL) {
            LOG_E("Could not create tone queue");
            return 1; // ERR
        }
        LOG_I("Tone queue created");
    }

    if (_tone_task == NULL) {
        LOG_I("Creating tone task");

        _tone_task = rt_thread_create(
                        "toneTask",                     // Name of the task
                        tone_task,                      // Function to implement the task
                        NULL,                           // Task input parameter
                        1024 * 8,                      // Stack size
                        RT_MAIN_THREAD_PRIORITY + 1,    // Priority of the task
                        10                              // Wait ticks
                        );

        if (_tone_task == NULL) {
            LOG_E("Could not create tone task");
            return 0; // ERR
        }

        rt_thread_startup(_tone_task);

        LOG_I("Tone task created");
    }

    return 0; // OK
}

void noTone(uint8_t _pin)
{
    LOG_D("noTone was called");

    if (tone_init()) {
        tone_msg_t tone_msg = {
            .tone_cmd = TONE_END,
            .pin = _pin,
            .frequency = 0, // Ignored
            .duration = 0,  // Ignored
        };

        rt_mq_send(_tone_queue, &tone_msg, sizeof(tone_msg_t));
    }
}

// parameters:
// _pin - pin number which will output the signal
// frequency - PWM frequency in Hz
// duration - time in ms - how long will the signal be outputted.
//   If not provided, or 0 you must manually call noTone to end output
void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
{
    LOG_D("_pin=%d, frequency=%u Hz, duration=%lu ms", _pin, frequency, duration);

    if (tone_init()) {
        tone_msg_t tone_msg = {
            .tone_cmd = TONE_START,
            .pin = _pin,
            .frequency = frequency,
            .duration = duration,
        };

        rt_mq_send(_tone_queue, &tone_msg, sizeof(tone_msg_t));
    }
}
