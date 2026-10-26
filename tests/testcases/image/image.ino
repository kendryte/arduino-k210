// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <Image.h>
#include <unity/unity.h>

using namespace K210;

void setUp(void) {}
void tearDown(void) {}

void test_color_layout_conversion(void)
{
    uint8_t rgb888[] = {
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 255,
    };
    const uint8_t expectedPlanar[] = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
    };

    Image source(2, 2, IMAGE_FORMAT_RGB888, rgb888);
    Image planar;
    TEST_ASSERT_EQUAL(0, source.to_rgbp888(&planar));
    TEST_ASSERT_EQUAL(2, planar.width());
    TEST_ASSERT_EQUAL(2, planar.height());
    TEST_ASSERT_EQUAL(12, planar.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedPlanar, planar.data(), sizeof(expectedPlanar));

    Image restored;
    TEST_ASSERT_EQUAL(0, planar.to_rgb888(&restored));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(rgb888, restored.data(), sizeof(rgb888));
}

void test_cut_and_imlib_view(void)
{
    uint8_t pixels[] = {
        1, 2, 3,
        4, 5, 6,
        7, 8, 9,
        10, 11, 12,
    };
    const uint8_t expected[] = {4, 5, 6, 10, 11, 12};
    Image source(2, 2, IMAGE_FORMAT_RGB888, pixels);
    Image cut;
    rectangle_t area = {1, 0, 1, 2};
    TEST_ASSERT_EQUAL(0, source.cut(&cut, area));
    TEST_ASSERT_EQUAL(1, cut.width());
    TEST_ASSERT_EQUAL(2, cut.height());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, cut.data(), sizeof(expected));

    image_t imlibImage = {};
    TEST_ASSERT_EQUAL(-1, cut.as_imlib_image(&imlibImage));

    Image grayscale;
    TEST_ASSERT_EQUAL(0, cut.to_grayscale(&grayscale));
    TEST_ASSERT_EQUAL(0, grayscale.as_imlib_image(&imlibImage));
    TEST_ASSERT_EQUAL(1, imlibImage.w);
    TEST_ASSERT_EQUAL(2, imlibImage.h);
    TEST_ASSERT_EQUAL(PIXFORMAT_GRAYSCALE, imlibImage.pixfmt);
    TEST_ASSERT_EQUAL_PTR(grayscale.data(), imlibImage.data);
}

void test_grayscale_and_invalid_source(void)
{
    uint8_t rgb888[] = {255, 255, 255, 0, 0, 0};
    Image source(2, 1, IMAGE_FORMAT_RGB888, rgb888);
    Image grayscale;
    TEST_ASSERT_EQUAL(0, source.to_grayscale(&grayscale));
    TEST_ASSERT_EQUAL(IMAGE_FORMAT_GRAYSCALE, grayscale.format());
    TEST_ASSERT_TRUE(grayscale.data()[0] > grayscale.data()[1]);

    Image invalid;
    Image destination;
    TEST_ASSERT_EQUAL(-1, invalid.to_rgb565(&destination));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_color_layout_conversion);
    RUN_TEST(test_cut_and_imlib_view);
    RUN_TEST(test_grayscale_and_invalid_source);
    UNITY_END();
}

void loop() {}
