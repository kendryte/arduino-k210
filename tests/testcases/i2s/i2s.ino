// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <I2S.h>
#include <unity/unity.h>

static volatile unsigned transmitCallbacks;

static void transmitted(void *context)
{
    volatile unsigned *count = static_cast<volatile unsigned *>(context);
    ++*count;
}

void setUp(void) {}
void tearDown(void) {}

void test_invalid_configuration(void)
{
    I2SClass invalidDevice(3, 0, 34, 32, 33);
    I2SClass duplicatePins(0, 0, 32, 32, 33);
    I2SClass audio(0, 0, 34, 32, 33);

    TEST_ASSERT_EQUAL(-1, invalidDevice.begin(I2S_PHILIPS_MODE, 16000, 16));
    TEST_ASSERT_EQUAL(-1, duplicatePins.begin(I2S_PHILIPS_MODE, 16000, 16));
    TEST_ASSERT_EQUAL(-1, audio.begin(I2S_PHILIPS_MODE, 0, 16));
    TEST_ASSERT_EQUAL(-1, audio.begin(I2S_PHILIPS_MODE, 193000, 16));
    TEST_ASSERT_EQUAL(-1, audio.begin(I2S_PHILIPS_MODE, 16000, 24));
    TEST_ASSERT_EQUAL(-1, audio.begin(99, 16000, 16));
    TEST_ASSERT_EQUAL(-1, audio.begin(I2S_PHILIPS_MODE, 16));
}

void test_controller_ownership(void)
{
    I2SClass first(1, 0, 34, 32, 33);
    I2SClass second(1, 0, 35, 30, 31);

    first.setBufferSize(32);
    second.setBufferSize(32);
    TEST_ASSERT_EQUAL(0, first.begin(I2S_LEFT_JUSTIFIED_MODE, 22050, 32));
    TEST_ASSERT_EQUAL(-1, second.begin(I2S_PHILIPS_MODE, 16000, 16));
    first.end();
    TEST_ASSERT_EQUAL(0, second.begin(I2S_PHILIPS_MODE, 16000, 16));
    second.end();
}

void test_transmit_dma(void)
{
    I2SClass audio(0, 0, 34, 32, 33);
    const uint32_t samples[] = {
        0x00010002, 0x00030004, 0x00050006, 0x00070008,
        0x0009000A, 0x000B000C, 0x000D000E, 0x000F0010,
    };

    transmitCallbacks = 0;
    audio.setBufferSize(sizeof(samples));
    TEST_ASSERT_EQUAL(sizeof(samples), audio.getBufferSize());
    TEST_ASSERT_EQUAL(0, audio.begin(I2S_PHILIPS_MODE, 16000, 16));
    audio.onTransmit(transmitted, const_cast<unsigned *>(&transmitCallbacks));
    TEST_ASSERT_EQUAL(sizeof(samples), audio.availableForWrite());
    TEST_ASSERT_EQUAL(sizeof(samples), audio.write(samples, sizeof(samples)));
    audio.flush();
    TEST_ASSERT_EQUAL(0, audio.getWriteError());
    TEST_ASSERT_TRUE(transmitCallbacks > 0);
    TEST_ASSERT_EQUAL(0, audio.available());
    audio.end();
}

void test_receive_requires_external_clock(void)
{
    TEST_IGNORE_MESSAGE("Connect an I2S source to add receive-path validation");
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_invalid_configuration);
    RUN_TEST(test_controller_ownership);
    RUN_TEST(test_transmit_dma);
    RUN_TEST(test_receive_requires_external_clock);
    UNITY_END();
}

void loop() {}
