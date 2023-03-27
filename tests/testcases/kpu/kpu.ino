#include "Arduino.h"

#include "FS.h"
#include "FFat.h"

#include "Image.h"
#include "KPU.h"

#include "unity/unity.h"

using namespace K210;

KPU_Yolo2 yolo2;

const char *obj_name[2] = {"no mask", "with mask"};
static float anchor[5 * 2] = {0.156250, 0.222548, 0.361328, 0.489583, 0.781250, 0.983133, 1.621094, 1.964286, 3.574219, 3.94000};

#define CUSTOM_IMG_W (320)
#define CUSTOM_IMG_H (256)

extern unsigned char face_data[];

/* These functions are intended to be called before and after each test. */
void setUp(void)
{
}

void tearDown(void)
{
}

int run_kmodel(void)
{
    obj_info_t info;

    Image *face_r8g8b8 = NULL, face_rgb888 = Image(320, 256, IMAGE_FORMAT_RGB888, face_data);

    face_r8g8b8 = face_rgb888.to_r8g8b8();
    if (NULL == face_r8g8b8)
    {
        Serial.println("convert pic failed\n");
        return -1;
    }

    if (!FFat.begin())
    {
        Serial.println("FFat Mount Failed");
        return -1;
    }

    if (0x00 != yolo2.load_kmodel(FFat, "/KPU/face_mask_detect/detect_5.kmodel"))
    {
        Serial.println("load kmodel failed\n");
        return -1;
    }

    if (0x00 != yolo2.begin(anchor, sizeof(anchor) / sizeof(float)))
    {
        Serial.println("yolo2 init failed\n");
        return -1;
    }

    if (0x00 != yolo2.run(face_r8g8b8, &info))
    {
        Serial.println("yolo2 run failed\n");
        return -1;
    }

    for (int i = 0; i < info.obj_number; i++)
    {
        Serial.printf("x %d y %d w %d h %d cls %s prob %d\n", info.obj[i].x, info.obj[i].y, info.obj[i].w, info.obj[i].h, obj_name[info.obj[i].class_id], int(info.obj[i].prob * 100));
    }

    return info.obj_number > 0 ? 0 : -1;
}

void test_kpu(void)
{
    TEST_ASSERT_EQUAL(0, run_kmodel());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }
    UNITY_BEGIN();
    RUN_TEST(test_kpu);
    UNITY_END();
}

void loop()
{
}
