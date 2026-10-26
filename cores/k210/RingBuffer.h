// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <stdint.h>
#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE 64
#endif
// Single producer / single consumer queue. clear() requires both sides to be quiescent.
template <int N> class RingBufferN {
    static_assert(N > 0, "ring buffer capacity must be positive");

public:
    void store_char(uint8_t value)
    {
        if (isFull())
            return;
        data_[write_] = value;
        write_        = (write_ + 1) % N;
        __atomic_add_fetch(&count_, 1, __ATOMIC_RELEASE);
    }
    int read_char()
    {
        if (!available())
            return -1;
        const uint8_t value = data_[read_];
        read_               = (read_ + 1) % N;
        __atomic_sub_fetch(&count_, 1, __ATOMIC_RELEASE);
        return value;
    }
    int peek()
    {
        return available() ? data_[read_] : -1;
    }
    int available()
    {
        return __atomic_load_n(&count_, __ATOMIC_ACQUIRE);
    }
    int availableForStore()
    {
        return N - available();
    }
    bool isFull()
    {
        return available() == N;
    }
    void clear()
    {
        read_ = write_ = 0;
        __atomic_store_n(&count_, 0, __ATOMIC_RELEASE);
    }

private:
    uint8_t  data_[N] = {};
    unsigned read_ = 0, write_ = 0;
    int      count_ = 0;
};
using RingBuffer = RingBufferN<SERIAL_BUFFER_SIZE>;
