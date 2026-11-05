// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors

#include <Arduino.h>
#include <k210-hal.h>
#include <unity/unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_channel_allocator(void)
{
    dmac_channel_number_t channels[4];

    for (size_t index = 0; index < 4; ++index) {
        channels[index] = hal_dma_get_free_chn();
        TEST_ASSERT_TRUE(channels[index] < DMAC_CHANNEL_MAX);
        for (size_t prior = 0; prior < index; ++prior) {
            TEST_ASSERT_NOT_EQUAL(channels[prior], channels[index]);
        }
    }
    TEST_ASSERT_EQUAL(DMAC_CHANNEL_MAX, hal_dma_get_free_chn());
    for (size_t index = 0; index < 4; ++index) {
        hal_dma_releas_chn(channels[index]);
    }
}

void test_memory_to_memory_transfer(void)
{
    alignas(8) const uint64_t source[] = {
        0x0123456789ABCDEFULL,
        0xFFEEDDCCBBAA9988ULL,
        0x13579BDF2468ACE0ULL,
        0x55AA55AAAA55AA55ULL,
    };
    alignas(8) uint64_t destination[4] = {};
    const dmac_channel_number_t channel = hal_dma_get_free_chn();

    TEST_ASSERT_TRUE(channel < DMAC_CHANNEL_MAX);
    dmac_set_single_mode(channel, source, destination,
                         DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_4, DMAC_TRANS_WIDTH_64,
                         sizeof(source) / sizeof(source[0]));
    dmac_wait_done(channel);
    TEST_ASSERT_EQUAL_UINT64_ARRAY(source, destination, sizeof(source) / sizeof(source[0]));
    hal_dma_releas_chn(channel);
}

void test_release_and_reuse(void)
{
    const dmac_channel_number_t first = hal_dma_get_free_chn();
    TEST_ASSERT_TRUE(first < DMAC_CHANNEL_MAX);
    hal_dma_releas_chn(first);
    const dmac_channel_number_t reused = hal_dma_get_free_chn();
    TEST_ASSERT_EQUAL(first, reused);
    hal_dma_releas_chn(reused);
    hal_dma_releas_chn(DMAC_CHANNEL_MAX);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {}

    UNITY_BEGIN();
    RUN_TEST(test_channel_allocator);
    RUN_TEST(test_memory_to_memory_transfer);
    RUN_TEST(test_release_and_reuse);
    UNITY_END();
}

void loop() {}
