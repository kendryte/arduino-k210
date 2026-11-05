// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <Wire.h>
#include <unity/unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_invalid_configuration(void)
{
    TwoWire invalidBus(3);
    TwoWire port(0);

    TEST_ASSERT_FALSE(invalidBus.begin(32, 33));
    TEST_ASSERT_FALSE(port.begin(-1, 33));
    TEST_ASSERT_FALSE(port.begin(32, 48));
    TEST_ASSERT_FALSE(port.begin(32, 32));
    TEST_ASSERT_FALSE(port.begin(32, 33, 0));
    TEST_ASSERT_FALSE(port.begin(uint16_t(0x48), 32, 33, 100000));
    TEST_ASSERT_EQUAL(0, port.write(uint8_t(0x55)));
    TEST_ASSERT_EQUAL(4, port.endTransmission());
    TEST_ASSERT_TRUE(port.end());
}

void test_buffer_overflow_and_invalid_requests(void)
{
    TwoWire port(0);
    uint8_t payload[HARDWARE_I2C_BUFF_LEN + 1] = {};

    TEST_ASSERT_TRUE(port.begin(32, 33, 100000));
    TEST_ASSERT_TRUE(port.begin(32, 33, 100000));
    TEST_ASSERT_FALSE(port.begin(32, 34, 100000));

    port.beginTransmission(0x48);
    TEST_ASSERT_EQUAL(HARDWARE_I2C_BUFF_LEN, port.write(payload, sizeof(payload)));
    TEST_ASSERT_TRUE(port.getWriteError());
    TEST_ASSERT_EQUAL(1, port.endTransmission());

    TEST_ASSERT_EQUAL(0, port.requestFrom(-1, 1));
    TEST_ASSERT_EQUAL(0, port.requestFrom(0x48, 0));
    TEST_ASSERT_EQUAL(0, port.available());
    TEST_ASSERT_EQUAL(-1, port.peek());
    TEST_ASSERT_EQUAL(-1, port.read());

    port.beginTransmission(-1);
    TEST_ASSERT_EQUAL(0, port.write(uint8_t(1)));
    TEST_ASSERT_EQUAL(4, port.endTransmission());
    port.flush();
    TEST_ASSERT_TRUE(port.end());
    TEST_ASSERT_TRUE(port.end());
}

void test_controller_ownership(void)
{
    TwoWire first(1);
    TwoWire second(1);

    TEST_ASSERT_TRUE(first.begin(32, 33, 400000));
    TEST_ASSERT_FALSE(second.begin(34, 35, 400000));
    TEST_ASSERT_TRUE(first.end());
    TEST_ASSERT_TRUE(second.begin(32, 33, 400000));
    TEST_ASSERT_TRUE(second.end());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_invalid_configuration);
    RUN_TEST(test_buffer_overflow_and_invalid_requests);
    RUN_TEST(test_controller_ownership);
    UNITY_END();
}

void loop() {}
