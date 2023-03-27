#include "Arduino.h"

#include "k210-hal.h"

#ifndef MAIN_STACK_SIZE
#define MAIN_STACK_SIZE 262144
#endif // MAIN_STACK_SIZE

extern int tone_init(void);

extern "C" int initPins(void);
extern "C" int initRtc(void);

static void user_main_thread(void *parameter)
{
    initPins();
    initRtc();

    tone_init();

    hal_flash_init(25 * 1000 * 1000, NULL);
    hal_flash_set_quad_mode();

    setup();
    while(1) {
        loop();
        rt_thread_delay(1);
    }
}

extern "C" int main(int argc, char *argv[])
{
    rt_thread_t thr = rt_thread_create(
                        "user_main",
                        user_main_thread, 
                        RT_NULL, 
                        MAIN_STACK_SIZE, 
                        RT_MAIN_THREAD_PRIORITY - 5, 
                        20);
    RT_ASSERT(thr != NULL);

	rt_thread_startup(thr);

    return 0;
}
