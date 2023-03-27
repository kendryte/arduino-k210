#include "Arduino.h"

#include "Image.h"
#include "GC0328.h"

#include "unity/unity.h"

using namespace K210;

GC0328 cam;

/* These functions are intended to be called before and after each test. */
void setUp(void) {
}

void tearDown(void){
}

static int snapshot()
{
    camera_buffers_t buff = {NULL, {NULL, NULL, NULL}};

    if (0x00 != cam.reset(FRAMESIZE_QVGA))
    {
        Serial.println("camera reset failed");
        return -1;
    }

    cam.get_buffers(&buff);
    if ((NULL == buff.disply) || (NULL == buff.ai.r))
    {
        Serial.println("get camera buffers failed");
        return -1;
    }

    if (0x00 != cam.snapshot())
    {
        Serial.println("camera snapshot failed");
        return -1;
    }

    return 0;
}

void test_camera()
{
    TEST_ASSERT_EQUAL(0, snapshot());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(test_camera);
    UNITY_END();
}

void loop()
{
}
