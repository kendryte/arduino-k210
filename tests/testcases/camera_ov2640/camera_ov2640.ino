// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <OV2640.h>
#include <unity/unity.h>

class ProbeOV2640 : public OV2640 {
public:
    bool beginSensorBus()
    {
        return _wire && _wire->begin(CAMERA_SCL_PIN, CAMERA_SDA_PIN, 400000);
    }
};

void setUp(void) {}
void tearDown(void) {}

void test_invalid_configuration(void)
{
    OV2640 invalid(-1, -1);
    TEST_ASSERT_EQUAL(-1, invalid.reset(FRAMESIZE_QVGA));
    TEST_ASSERT_EQUAL(-1, invalid.reset(FRAMESIZE_INVALID));
}

void test_ov2640_if_present(void)
{
    ProbeOV2640 camera;
    TEST_ASSERT_TRUE(camera.beginSensorBus());
    const int sensorId = camera.read_id();
    if (sensorId != 0x2642) {
        TEST_IGNORE_MESSAGE("OV2640 not detected; install that sensor to run this case");
    }

    TEST_ASSERT_EQUAL(0, camera.reset(FRAMESIZE_QVGA));
    TEST_ASSERT_EQUAL(320, camera.width());
    TEST_ASSERT_EQUAL(240, camera.height());
    TEST_ASSERT_EQUAL(0, camera.snapshot(500));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_invalid_configuration);
    RUN_TEST(test_ov2640_if_present);
    UNITY_END();
}

void loop() {}
