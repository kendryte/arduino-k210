// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <Bus8080.h>
#include <ST7789V.h>
#include <unity/unity.h>

static ST7789V display(240, 320);
static uint16_t frameBuffer[240 * 320];

void setUp(void) {}
void tearDown(void) {}

void test_rotation_geometry(void)
{
    display.setRotation(0);
    TEST_ASSERT_EQUAL(240, display.width());
    TEST_ASSERT_EQUAL(320, display.height());
    display.setRotation(1);
    TEST_ASSERT_EQUAL(320, display.width());
    TEST_ASSERT_EQUAL(240, display.height());
    display.setRotation(2);
    TEST_ASSERT_EQUAL(240, display.width());
    TEST_ASSERT_EQUAL(320, display.height());
    display.setRotation(3);
    TEST_ASSERT_EQUAL(320, display.width());
    TEST_ASSERT_EQUAL(240, display.height());
    display.setRotation(0);
}

void test_framebuffer_contract(void)
{
    TEST_ASSERT_EQUAL(-1, display.setFrameBuffer(static_cast<uint16_t *>(nullptr), 240, 320));
    TEST_ASSERT_EQUAL(-1, display.setFrameBuffer(frameBuffer, 239, 320));
    TEST_ASSERT_EQUAL(0, display.setFrameBuffer(frameBuffer, 240, 320));

    memset(frameBuffer, 0, sizeof(frameBuffer));
    display.drawPixel(7, 9, 0xF81F);
    TEST_ASSERT_EQUAL_HEX16(0xF81F, frameBuffer[9 * 240 + 7]);
    display.drawPixel(-1, 0, 0xFFFF);
    display.drawPixel(240, 0, 0xFFFF);
    TEST_ASSERT_EQUAL_HEX16(0, frameBuffer[0]);
}

void test_bitmap_and_bus_refresh(void)
{
    const uint16_t pixels[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
    memset(frameBuffer, 0, sizeof(frameBuffer));
    display.drawFastRGBBitmap(10, 12, 2, 2, const_cast<uint16_t *>(pixels));
    TEST_ASSERT_EQUAL_HEX16(0xF800, frameBuffer[12 * 240 + 10]);
    TEST_ASSERT_EQUAL_HEX16(0x07E0, frameBuffer[12 * 240 + 11]);
    TEST_ASSERT_EQUAL_HEX16(0x001F, frameBuffer[13 * 240 + 10]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, frameBuffer[13 * 240 + 11]);

    display.refresh();
    display.invertDisplay(true);
    display.invertDisplay(false);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}
    display.begin();

    UNITY_BEGIN();
    RUN_TEST(test_rotation_geometry);
    RUN_TEST(test_framebuffer_contract);
    RUN_TEST(test_bitmap_and_bus_refresh);
    K210::Bus8080::end();
    UNITY_END();
}

void loop() {}
