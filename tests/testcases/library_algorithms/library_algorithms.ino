// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <I2SDoubleBuffer.h>
#include <KPU.h>
#include <ScopedTimer.h>
#include <TouchScreen.h>
#include <imlib.h>
#include <unity/unity.h>

using namespace K210;

void setUp(void) {}
void tearDown(void) {}

void test_i2s_double_buffer(void)
{
    I2SDoubleBuffer buffer;
    buffer.setSize(10);
    TEST_ASSERT_EQUAL(8, buffer.getSize());
    buffer.reset();
    TEST_ASSERT_NOT_NULL(buffer.data());

    const uint8_t input[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    TEST_ASSERT_EQUAL(8, buffer.write(input, sizeof(input)));
    buffer.swap();
    TEST_ASSERT_EQUAL(8, buffer.availableForWrite());

    memcpy(buffer.data(), input, 8);
    buffer.swap(8);
    uint8_t output[8] = {};
    TEST_ASSERT_EQUAL(8, buffer.read(output, sizeof(output)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, output, sizeof(output));
    TEST_ASSERT_EQUAL(0, buffer.available());
}

void test_touch_filters(void)
{
    mean_filter_t *mean = mean_alloc(3);
    median_filter_t *median = median_alloc(3);
    TEST_ASSERT_NOT_NULL(mean);
    TEST_ASSERT_NOT_NULL(median);

    TEST_ASSERT_EQUAL(3, mean_update(mean, 3));
    TEST_ASSERT_EQUAL(4, mean_update(mean, 5));
    TEST_ASSERT_EQUAL(5, mean_update(mean, 7));
    TEST_ASSERT_EQUAL(7, median_update(median, 7));
    TEST_ASSERT_EQUAL(1, median_update(median, 1));
    TEST_ASSERT_EQUAL(5, median_update(median, 5));

    mean_free(mean);
    median_free(median);

    tsfilter_t *filter = tsfilter_alloc(1, 1);
    TEST_ASSERT_NOT_NULL(filter);
    int calibration[] = {2, 0, 10, 0, 3, -4, 1};
    tsfilter_setcal(filter, calibration);
    int x = 5;
    int y = 6;
    tsfilter_update(filter, &x, &y);
    TEST_ASSERT_EQUAL(20, x);
    TEST_ASSERT_EQUAL(14, y);
    tsfilter_free(filter);
}

void test_imlib_geometry(void)
{
    rectangle_t first;
    rectangle_t second;
    rectangle_init(&first, 0, 0, 10, 10);
    rectangle_init(&second, 5, 5, 10, 10);
    TEST_ASSERT_TRUE(rectangle_overlap(&first, &second));

    rectangle_intersected(&first, &second);
    TEST_ASSERT_EQUAL(5, first.x);
    TEST_ASSERT_EQUAL(5, first.y);
    TEST_ASSERT_EQUAL(5, first.w);
    TEST_ASSERT_EQUAL(5, first.h);

    rectangle_t separate;
    rectangle_init(&separate, 30, 30, 2, 2);
    TEST_ASSERT_FALSE(rectangle_overlap(&first, &separate));
}

void test_kpu_activation(void)
{
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, KPU_Activation::sigmoid(0.0f));
    float input[] = {1.0f, 2.0f, 3.0f};
    float output[3] = {};
    KPU_Activation::softmax(input, output, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, output[0] + output[1] + output[2]);
    TEST_ASSERT_TRUE(output[0] < output[1] && output[1] < output[2]);
}

void test_scoped_timer(void)
{
    const uint64_t start = sysctl_get_time_us();
    {
        ScopedTimer timer("library_algorithms", false);
        delay(10);
    }
    TEST_ASSERT_TRUE(sysctl_get_time_us() > start);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_i2s_double_buffer);
    RUN_TEST(test_touch_filters);
    RUN_TEST(test_imlib_geometry);
    RUN_TEST(test_kpu_activation);
    RUN_TEST(test_scoped_timer);
    UNITY_END();
}

void loop() {}
