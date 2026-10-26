// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "I2SDoubleBuffer.h"
#include <Arduino.h>
#define I2S_HAS_SET_BUFFER_SIZE 1

enum i2s_mode_t {
    I2S_PHILIPS_MODE         = STANDARD_MODE,
    I2S_RIGHT_JUSTIFIED_MODE = RIGHT_JUSTIFYING_MODE,
    I2S_LEFT_JUSTIFIED_MODE  = LEFT_JUSTIFYING_MODE
};
// One task owns each instance. onReceive/onTransmit callbacks execute in DMA IRQs.
class I2SClass : public Stream {
public:
    using callback_t          = void (*)();
    using callback_with_arg_t = void (*)(void*);
    I2SClass(uint8_t device, uint8_t clockGenerator, uint8_t sd, uint8_t sck, uint8_t fs);
    ~I2SClass() override
    {
        end();
    }
    I2SClass(const I2SClass&) = delete;
    I2SClass& operator=(const I2SClass&) = delete;
    // Return 0 on success, -1 on invalid configuration or unavailable resources.
    int       begin(int mode, long sampleRate, int bitsPerSample);
    int       begin(int mode, int bitsPerSample);
    void      end();
    int       available() override;
    int       availableForWrite() override;
    int       read() override;
    int       peek() override;
    void      flush() override;
    int       read(void* buffer, size_t size);
    size_t    write(uint8_t value) override;
    size_t    write(const uint8_t* buffer, size_t size) override;
    size_t    write(const void* buffer, size_t size);
    size_t    write(int value);
    size_t    write(uint32_t value);
    void      onTransmit(callback_with_arg_t cb, void* arg);
    void      onReceive(callback_with_arg_t cb, void* arg);
    void      onTransmit(callback_t cb);
    void      onReceive(callback_t cb);
    void      setBufferSize(int size);
    int       getBufferSize()
    {
        return buffer_.getSize();
    }
    void _onTransferComplete();

private:
    enum Direction { Idle, Sending, Receiving };
    uint8_t               device_, clock_, sd_, sck_, fs_;
    bool                  ready_     = false;
    Direction             direction_ = Idle;
    dmac_channel_number_t dma_       = DMAC_CHANNEL_MAX;
    uint32_t              rate_      = 0;
    int                   bits_ = 0, mode_ = 0;
    volatile bool         busy_     = false;
    bool                  received_ = false;
    I2SDoubleBuffer       buffer_;
    callback_with_arg_t   tx_ = nullptr, rx_ = nullptr;
    callback_t            txSimple_ = nullptr, rxSimple_ = nullptr;
    void *                txArg_ = nullptr, *rxArg_ = nullptr;
    bool                  select(Direction direction);
    void                  service();
    static int            completed(void* context);
};
