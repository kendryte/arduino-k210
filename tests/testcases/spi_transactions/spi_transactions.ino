// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <SPI.h>
#include <unity/unity.h>

// No loopback wiring is required: exercise transaction ownership on two tasks.
struct SetterTask {
    SPIClass* port;
    unsigned setter;
    rt_sem_t started;
    rt_sem_t done;
};

static void change_setting(void* argument)
{
    SetterTask* task = static_cast<SetterTask*>(argument);
    rt_sem_release(task->started);
    if (task->setter == 0)
        task->port->setFrequency(250000);
    else if (task->setter == 1)
        task->port->setDataMode(SPI_MODE1);
    else
        task->port->setBitOrder(SPI_LSBFIRST);
    rt_sem_release(task->done);
}

void setUp(void) {}
void tearDown(void) {}

static void check_setter(unsigned setter)
{
    SPIClass port(SPI_BUS_1);
    SetterTask task = {&port, setter, rt_sem_create("spiStart", 0, RT_IPC_FLAG_FIFO),
                       rt_sem_create("spiDone", 0, RT_IPC_FLAG_FIFO)};
    if (!task.started || !task.done) {
        if (task.started) rt_sem_delete(task.started);
        if (task.done) rt_sem_delete(task.done);
        TEST_FAIL_MESSAGE("Cannot allocate SPI test semaphores");
    }
    rt_thread_t worker = rt_thread_create("spiSet", change_setting, &task, 4096, 15, 10);
    if (!worker) {
        rt_sem_delete(task.started);
        rt_sem_delete(task.done);
        TEST_FAIL_MESSAGE("Cannot allocate SPI test thread");
    }
    port.begin(32, 35, 34, 33);
    port.beginTransaction(SPISettings(1000000, SPI_MSBFIRST, SPI_MODE0));
    // Setters must also work recursively for the transaction owner.
    port.setFrequency(500000);
    port.setDataMode(SPI_MODE3);
    port.setBitOrder(SPI_MSBFIRST);
    const uint32_t divider = spi[SPI_BUS_1]->baudr;
    const uint32_t control = spi[SPI_BUS_1]->ctrlr0;
    rt_thread_startup(worker);
    const bool started = rt_sem_take(task.started, rt_tick_from_millisecond(1000)) == RT_EOK;
    const bool blocked = rt_sem_take(task.done, rt_tick_from_millisecond(50)) == -RT_ETIMEOUT;
    const bool preserved = spi[SPI_BUS_1]->baudr == divider && spi[SPI_BUS_1]->ctrlr0 == control;
    port.endTransaction();
    const bool completed = !blocked || rt_sem_take(task.done, rt_tick_from_millisecond(1000)) == RT_EOK;
    if (!completed) rt_thread_delete(worker);
    const bool applied = setter == 0 ? spi[SPI_BUS_1]->baudr > divider
                        : setter == 1 ? ((spi[SPI_BUS_1]->ctrlr0 >> 6) & 3) == SPI_MODE1 : true;
    port.end();
    rt_sem_delete(task.started);
    rt_sem_delete(task.done);
    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_TRUE(blocked);
    TEST_ASSERT_TRUE(preserved);
    TEST_ASSERT_TRUE(completed);
    TEST_ASSERT_TRUE(applied);
}

void test_frequency_waits_for_transaction(void) { check_setter(0); }
void test_mode_waits_for_transaction(void) { check_setter(1); }
void test_bit_order_waits_for_transaction(void) { check_setter(2); }

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}
    UNITY_BEGIN();
    RUN_TEST(test_frequency_waits_for_transaction);
    RUN_TEST(test_mode_waits_for_transaction);
    RUN_TEST(test_bit_order_waits_for_transaction);
    UNITY_END();
}

void loop() {}
