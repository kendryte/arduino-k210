// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <stddef.h>
#include <stdint.h>
#define DEFAULT_I2S_BUFFER_SIZE 2048

// Two aligned storage slots. The caller serializes access with its DMA interrupt.
// read/write/available address the application slot; data() addresses the DMA slot.
class I2SDoubleBuffer {
public:
    I2SDoubleBuffer() = default;
    virtual ~I2SDoubleBuffer()
    {
        end();
    }
    I2SDoubleBuffer(const I2SDoubleBuffer&) = delete;
    I2SDoubleBuffer& operator=(const I2SDoubleBuffer&) = delete;
    void             setSize(int size);
    int              getSize()
    {
        return capacity_;
    }
    void   reset();
    void   end();
    size_t availableForWrite();
    size_t write(const void* buffer, size_t size);
    size_t read(void* buffer, size_t size);
    size_t peek(void* buffer, size_t size);
    void*  data();
    size_t available();
    void   swap(int length = 0);
    void   flush();

private:
    uint8_t* storage_     = nullptr;
    int      capacity_    = DEFAULT_I2S_BUFFER_SIZE;
    unsigned application_ = 0;
    size_t   used_ = 0, consumed_ = 0;
};
