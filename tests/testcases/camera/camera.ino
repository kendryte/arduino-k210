#include "Arduino.h"

#include "Image.h"
#include "GC0328.h"

#include "unity/unity.h"

using namespace K210;

GC0328 cam;
static bool cameraReady;

/* These functions are intended to be called before and after each test. */
void setUp(void) {
}

void tearDown(void){
}

static int initialize_camera()
{
    camera_buffers_t buff = {NULL, {NULL, NULL, NULL}};

    if (!cameraReady && 0x00 != cam.reset(FRAMESIZE_QVGA))
    {
        Serial.println("camera reset failed");
        return -1;
    }
    cameraReady = true;

    cam.get_buffers(&buff);
    if ((NULL == buff.disply) || (NULL == buff.ai.r) ||
        (NULL == buff.ai.g) || (NULL == buff.ai.b))
    {
        Serial.println("get camera buffers failed");
        return -1;
    }

    return 0;
}

void test_camera_initialization()
{
    TEST_ASSERT_EQUAL(-1, cam.reset(FRAMESIZE_INVALID));
    TEST_ASSERT_EQUAL(0, initialize_camera());
    TEST_ASSERT_EQUAL(320, cam.width());
    TEST_ASSERT_EQUAL(240, cam.height());
    TEST_ASSERT_TRUE(cam.read_id() > 0);
}

void test_camera_controls()
{
    TEST_ASSERT_TRUE(cameraReady);
    TEST_ASSERT_EQUAL(0, cam.set_hmirror(1));
    TEST_ASSERT_TRUE(cam.mirror());
    TEST_ASSERT_EQUAL(0, cam.set_hmirror(0));
    TEST_ASSERT_FALSE(cam.mirror());
    TEST_ASSERT_EQUAL(0, cam.set_vflip(1));
    TEST_ASSERT_TRUE(cam.flip());
    TEST_ASSERT_EQUAL(0, cam.set_vflip(0));
    TEST_ASSERT_FALSE(cam.flip());
    TEST_ASSERT_EQUAL(-1, cam.set_brightness(0));
    TEST_ASSERT_EQUAL(-1, cam.set_colorbar(1));
}

void test_camera_snapshots()
{
    TEST_ASSERT_EQUAL(0, initialize_camera());

    TEST_ASSERT_EQUAL(0, cam.snapshot(500, true));
    TEST_ASSERT_EQUAL(0, cam.snapshot(500, false));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(test_camera_initialization);
    RUN_TEST(test_camera_controls);
    RUN_TEST(test_camera_snapshots);
    UNITY_END();
}

void loop()
{
}
