// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <SPI.h>
#include <unity/unity.h>

static const int PIN_SCK = 32;
static const int PIN_CS = 33;
static const int PIN_MOSI = 34;
static const int PIN_MISO = 35;

static SPIClass loopback(SPI_BUS_1);

void setUp(void)
{
    loopback.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
    loopback.beginTransaction(SPISettings(1000000, SPI_MSBFIRST, SPI_MODE0));
}

void tearDown(void)
{
    loopback.endTransaction();
    loopback.end();
}

void test_byte_loopback(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xA5, loopback.transfer(0xA5));
    TEST_ASSERT_EQUAL_HEX8(0x3C, loopback.transfer(0x3C));
}

void test_word_loopback(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x1234, loopback.transfer16(0x1234));
    TEST_ASSERT_EQUAL_HEX32(0x89ABCDEF, loopback.transfer32(0x89ABCDEF));
}

void test_buffer_loopback(void)
{
    uint8_t buffer[] = {0x00, 0x55, 0xAA, 0xFF, 0x31, 0x7E};
    const uint8_t expected[] = {0x00, 0x55, 0xAA, 0xFF, 0x31, 0x7E};
    loopback.transfer(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buffer, sizeof(buffer));
}

void test_lsb_first_loopback(void)
{
    loopback.endTransaction();
    loopback.beginTransaction(SPISettings(500000, SPI_LSBFIRST, SPI_MODE0));
    TEST_ASSERT_EQUAL_HEX8(0x96, loopback.transfer(0x96));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_byte_loopback);
    RUN_TEST(test_word_loopback);
    RUN_TEST(test_buffer_loopback);
    RUN_TEST(test_lsb_first_loopback);
    UNITY_END();
}

void loop() {}
