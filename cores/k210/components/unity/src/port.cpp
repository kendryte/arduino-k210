/*
 * SPDX-FileCopyrightText: 2016-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdbool.h>

#include "unity/unity.h"

#include "k210-hal.h"

#include "HardwareSerial.h"

extern "C"
{

static uint64_t s_test_start, s_test_stop;

void unity_putc(int c)
{
    if (c == '\n') {
        Serial.write('\r');
        Serial.write('\n');
    } else if (c == '\r') {
    } else {
        Serial.write(c);
    }
}

void unity_flush(void)
{

}

void unity_exec_time_start(void)
{
    s_test_start = sysctl_get_time_us();
}

void unity_exec_time_stop(void)
{
    s_test_stop = sysctl_get_time_us();
}

uint64_t unity_exec_time_get_ms(void)
{
    return (s_test_stop - s_test_start) / 1000;
}

}
