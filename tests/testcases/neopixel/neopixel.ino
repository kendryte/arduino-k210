// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <NoePixelWrap.h>
#include <unity/unity.h>

static Adafruit_NeoPixel_Warp pixels(4, 18, NEO_GRB + NEO_KHZ800);

void setUp(void)
{
    pixels.clear();
}

void tearDown(void) {}

void test_color_buffer(void)
{
    const uint32_t first = pixels.Color(0x12, 0x34, 0x56);
    const uint32_t second = pixels.Color(0xAA, 0xBB, 0xCC);

    pixels.setPixelColor(0, first);
    pixels.setPixelColor(3, second);
    TEST_ASSERT_EQUAL_HEX32(first, pixels.getPixelColor(0));
    TEST_ASSERT_EQUAL_HEX32(second, pixels.getPixelColor(3));
    TEST_ASSERT_EQUAL_HEX32(0, pixels.getPixelColor(1));
}

void test_clear_and_out_of_range(void)
{
    pixels.fill(pixels.Color(1, 2, 3));
    pixels.setPixelColor(99, pixels.Color(255, 255, 255));
    pixels.clear();
    for (uint16_t index = 0; index < pixels.numPixels(); ++index) {
        TEST_ASSERT_EQUAL_HEX32(0, pixels.getPixelColor(index));
    }
}

void test_k210_signal_output(void)
{
    pixels.setPixelColor(0, pixels.Color(16, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 16, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 16));
    pixels.setPixelColor(3, pixels.Color(8, 8, 8));
    pixels.show();
    TEST_ASSERT_EQUAL_HEX32(pixels.Color(16, 0, 0), pixels.getPixelColor(0));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}
    pixels.begin();

    UNITY_BEGIN();
    RUN_TEST(test_color_buffer);
    RUN_TEST(test_clear_and_out_of_range);
    RUN_TEST(test_k210_signal_output);
    UNITY_END();
}

void loop() {}
