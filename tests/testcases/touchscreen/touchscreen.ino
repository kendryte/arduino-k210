// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <NS2009.h>
#include <Wire.h>
#include <ts_lib/ts_lib.h>
#include <unity/unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_event_transitions(void)
{
    ts_event_t event = {};

    push_event_begin(&event, 10, 20);
    TEST_ASSERT_EQUAL(TOUCH_BEGIN, event.type);
    TEST_ASSERT_EQUAL(10, event.x);
    TEST_ASSERT_EQUAL(20, event.y);
    push_event_move(&event, 30, 40);
    TEST_ASSERT_EQUAL(TOUCH_MOVE, event.type);
    push_event_end(&event, 50, 60);
    TEST_ASSERT_EQUAL(TOUCH_END, event.type);
    push_event_none(&event);
    TEST_ASSERT_EQUAL(TOUCH_NONE, event.type);
    TEST_ASSERT_EQUAL(0, event.x);
    TEST_ASSERT_EQUAL(0, event.y);
}

void test_calibration_solver(void)
{
    tscal_t calibration = {};
    const int rawX[] = {0, 10, 10, 0, 5};
    const int rawY[] = {0, 0, 10, 10, 5};

    for (size_t index = 0; index < 5; ++index) {
        calibration.x[index] = rawX[index];
        calibration.y[index] = rawY[index];
        calibration.xfb[index] = 2 * rawX[index] + 3 * rawY[index] + 5;
        calibration.yfb[index] = -rawX[index] + 4 * rawY[index] + 7;
    }
    TEST_ASSERT_EQUAL(1, perform_calibration(&calibration));
    const int mappedX = (calibration.a[2] + calibration.a[0] * 4 + calibration.a[1] * 6) / calibration.a[6];
    const int mappedY = (calibration.a[5] + calibration.a[3] * 4 + calibration.a[4] * 6) / calibration.a[6];
    TEST_ASSERT_INT_WITHIN(1, 31, mappedX);
    TEST_ASSERT_INT_WITHIN(1, 27, mappedY);

    tscal_t degenerate = {};
    TEST_ASSERT_EQUAL(0, perform_calibration(&degenerate));
}

void test_ns2009_if_present(void)
{
    TwoWire touchBus(1);
    TEST_ASSERT_TRUE(touchBus.begin(30, 31, 400000));
    NS2009 touch(&touchBus);
    if (!touch.begin()) {
        touchBus.end();
        TEST_IGNORE_MESSAGE("NS2009 not detected on GPIO 30/31");
    }

    const TSPoint point = touch.poll();
    TEST_ASSERT_TRUE(point.type >= TOUCH_NONE && point.type <= TOUCH_END);
    touchBus.end();
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_event_transitions);
    RUN_TEST(test_calibration_solver);
    RUN_TEST(test_ns2009_if_present);
    UNITY_END();
}

void loop() {}
