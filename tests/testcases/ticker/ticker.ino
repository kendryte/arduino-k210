// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <Ticker.h>
#include <unity/unity.h>

static volatile unsigned callbackCount;

void setUp(void)
{
    callbackCount = 0;
}

void tearDown(void) {}

static void increment(void)
{
    ++callbackCount;
}

static void add(unsigned amount)
{
    callbackCount += amount;
}

void test_once(void)
{
    Ticker ticker;
    ticker.once_ms(20, increment);
    TEST_ASSERT_TRUE(ticker.active());
    delay(80);
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_FALSE(ticker.active());
}

void test_repeat_and_detach(void)
{
    Ticker ticker;
    ticker.attach_ms(10, increment);
    delay(80);
    ticker.detach();
    const unsigned stoppedAt = callbackCount;
    TEST_ASSERT_TRUE(stoppedAt >= 3);
    TEST_ASSERT_FALSE(ticker.active());
    delay(30);
    TEST_ASSERT_EQUAL(stoppedAt, callbackCount);
}

void test_typed_argument(void)
{
    Ticker ticker;
    ticker.once_ms(10, add, 7U);
    delay(50);
    TEST_ASSERT_EQUAL(7, callbackCount);
}

void test_invalid_interval(void)
{
    Ticker ticker;
    ticker.attach_ms(0, increment);
    TEST_ASSERT_FALSE(ticker.active());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_once);
    RUN_TEST(test_repeat_and_detach);
    RUN_TEST(test_typed_argument);
    RUN_TEST(test_invalid_interval);
    UNITY_END();
}

void loop() {}
