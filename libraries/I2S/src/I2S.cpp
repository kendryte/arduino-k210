// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "I2S.h"
#include "rthw.h"
namespace {
I2SClass* owners[3] = {};
class InterruptGuard {
    rt_base_t level_ = rt_hw_interrupt_disable();

public:
    ~InterruptGuard()
    {
        rt_hw_interrupt_enable(level_);
    }
};
}
I2SClass::I2SClass(uint8_t device, uint8_t clockGenerator, uint8_t sd, uint8_t sck, uint8_t fs)
    : device_(device)
    , clock_(clockGenerator)
    , sd_(sd)
    , sck_(sck)
    , fs_(fs)
{
}
int I2SClass::begin(int mode, int bitsPerSample)
{
    // This SDK configures master clocks only. Do not claim a successful slave setup.
    (void)mode;
    (void)bitsPerSample;
    return -1;
}
int I2SClass::begin(int mode, long sampleRate, int bitsPerSample)
{
    if (ready_ || device_ >= 3 || sampleRate <= 0 || sampleRate > 192000 || (bitsPerSample != 16 && bitsPerSample != 32)
        || (mode != I2S_PHILIPS_MODE && mode != I2S_LEFT_JUSTIFIED_MODE && mode != I2S_RIGHT_JUSTIFIED_MODE)
        || sd_ >= K210_IO_NUMS || sck_ >= K210_IO_NUMS || fs_ >= K210_IO_NUMS || sd_ == sck_ || sd_ == fs_ || sck_ == fs_)
        return -1;
    I2SClass* expected = nullptr;
    if (!__atomic_compare_exchange_n(&owners[device_], &expected, this, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        return -1;
    buffer_.reset();
    dma_ = buffer_.data() ? hal_dma_get_free_chn() : DMAC_CHANNEL_MAX;
    if (dma_ == DMAC_CHANNEL_MAX) {
        buffer_.end();
        __atomic_store_n(&owners[device_], static_cast<I2SClass*>(nullptr), __ATOMIC_RELEASE);
        return -1;
    }
    rate_      = sampleRate;
    bits_      = bitsPerSample;
    mode_      = mode;
    direction_ = Idle;
    busy_ = received_ = false;
    ready_            = true;
    (void)clock_; // Kept in the API; K210 uses its device clock divider.
    return 0;
}
bool I2SClass::select(Direction direction)
{
    if (!ready_ || (direction_ != Idle && direction_ != direction))
        return false;
    if (direction_ == direction)
        return true;
    direction_         = direction;
    const auto device  = static_cast<i2s_device_number_t>(device_);
    const int  routing = device_ * 11;
    hal_fpioa_set_pin_func(sck_, static_cast<fpioa_function_t>(FUNC_I2S0_SCLK + routing));
    hal_fpioa_set_pin_func(fs_, static_cast<fpioa_function_t>(FUNC_I2S0_WS + routing));
    hal_fpioa_set_pin_func(
        sd_, static_cast<fpioa_function_t>((direction == Sending ? FUNC_I2S0_OUT_D0 : FUNC_I2S0_IN_D0) + routing));
    i2s_init(device, direction == Sending ? I2S_TRANSMITTER : I2S_RECEIVER, 3);
    const auto width = bits_ == 16 ? RESOLUTION_16_BIT : RESOLUTION_32_BIT;
    if (direction == Sending)
        i2s_tx_channel_config(device, I2S_CHANNEL_0, width, SCLK_CYCLES_32, TRIGGER_LEVEL_4,
                              static_cast<i2s_work_mode_t>(mode_));
    else
        i2s_rx_channel_config(device, I2S_CHANNEL_0, width, SCLK_CYCLES_32, TRIGGER_LEVEL_4,
                              static_cast<i2s_work_mode_t>(mode_));
    i2s_set_dma_divide_16(device, bits_ == 16);
    i2s_set_sample_rate(device, rate_);
    return true;
}
void I2SClass::service()
{
    if (!ready_ || busy_ || direction_ == Idle)
        return;
    i2s_data_t transfer = {};
    transfer.mode       = direction_ == Sending ? I2S_TRANSMITTER : I2S_RECEIVER;
    if (direction_ == Sending) {
        const size_t count = buffer_.available();
        // DMA transfers whole 32-bit words. Keep partial words for the next write.
        if (!count || count % 4)
            return;
        buffer_.swap();
        transfer.send.tx_channel = dma_;
        transfer.send.tx_buf     = static_cast<uint32_t*>(buffer_.data());
        transfer.send.tx_len     = count / 4;
    } else {
        if (received_) {
            if (buffer_.available())
                return; // Preserve unread samples; backpressure DMA.
            buffer_.swap(buffer_.getSize());
            received_ = false;
        }
        transfer.recv.rx_channel = dma_;
        transfer.recv.rx_buf     = static_cast<uint32_t*>(buffer_.data());
        transfer.recv.rx_len     = buffer_.getSize() / 4;
    }
    plic_interrupt_t interrupt = {};
    interrupt.callback         = completed;
    interrupt.ctx              = this;
    interrupt.priority         = 1;
    busy_                      = true;
    i2s_handle_data_dma(static_cast<i2s_device_number_t>(device_), transfer, &interrupt);
}
int I2SClass::available()
{
    InterruptGuard guard;
    if (!select(Receiving))
        return 0;
    service();
    return buffer_.available();
}
int I2SClass::availableForWrite()
{
    InterruptGuard guard;
    if (!select(Sending))
        return 0;
    return buffer_.availableForWrite();
}
int I2SClass::read(void* output, size_t size)
{
    if (!output)
        return 0;
    InterruptGuard guard;
    if (!select(Receiving))
        return 0;
    service();
    const int count = buffer_.read(output, size);
    service();
    return count;
}
int I2SClass::read()
{
    uint32_t     sample = 0;
    const size_t width  = bits_ == 16 ? 2 : 4;
    if (available() < static_cast<int>(width))
        return -1;
    if (read(&sample, width) != static_cast<int>(width))
        return -1;
    return bits_ == 16 ? static_cast<int16_t>(sample) : static_cast<int32_t>(sample);
}
int I2SClass::peek()
{
    InterruptGuard guard;
    if (!select(Receiving))
        return -1;
    service();
    const size_t width  = bits_ == 16 ? 2 : 4;
    uint32_t     sample = 0;
    if (buffer_.available() < width || buffer_.peek(&sample, width) != width)
        return -1;
    return bits_ == 16 ? static_cast<int16_t>(sample) : static_cast<int32_t>(sample);
}
size_t I2SClass::write(const uint8_t* input, size_t size)
{
    return write(static_cast<const void*>(input), size);
}
size_t I2SClass::write(const void* input, size_t size)
{
    if (!input || !size)
        return 0;
    InterruptGuard guard;
    if (!select(Sending))
        return 0;
    const size_t count = buffer_.write(input, size);
    service();
    return count;
}
size_t I2SClass::write(uint8_t value)
{
    return write(static_cast<uint32_t>(value));
}
size_t I2SClass::write(int value)
{
    return write(static_cast<uint32_t>(value));
}
size_t I2SClass::write(uint32_t value)
{
    return write(&value, bits_ == 16 ? 2 : 4);
}
void I2SClass::flush()
{
    if (!ready_)
        return;
    if (direction_ == Receiving) {
        InterruptGuard guard;
        buffer_.flush();
        service();
        return;
    }
    const unsigned long started = millis();
    for (;;) {
        {
            InterruptGuard guard;
            const uint8_t  zero[3]   = {};
            const size_t   remainder = buffer_.available() % 4;
            if (remainder)
                buffer_.write(zero, 4 - remainder);
            service();
            if (!busy_ && !buffer_.available())
                break;
        }
        if (static_cast<unsigned long>(millis() - started) >= 1000) {
            setWriteError();
            break;
        }
        yield();
    }
}
void I2SClass::end()
{
    if (!ready_)
        return;
    if (direction_ == Sending)
        flush();
    InterruptGuard guard;
    ready_ = false;
    dmac_irq_unregister(dma_);
    dmac_channel_disable(dma_);
    dmac_wait_idle(dma_);
    hal_dma_releas_chn(dma_);
    dma_ = DMAC_CHANNEL_MAX;
    if (direction_ != Idle) {
        // Stop both data blocks and their clock before freeing DMA memory.
        i2s[device_]->ier = 0;
        hal_fpioa_clr_pin_func(sd_);
        hal_fpioa_clr_pin_func(sck_);
        hal_fpioa_clr_pin_func(fs_);
    }
    busy_ = received_ = false;
    direction_        = Idle;
    buffer_.end();
    __atomic_store_n(&owners[device_], static_cast<I2SClass*>(nullptr), __ATOMIC_RELEASE);
}
int I2SClass::completed(void* context)
{
    static_cast<I2SClass*>(context)->_onTransferComplete();
    return 0;
}
void I2SClass::_onTransferComplete()
{
    if (!ready_)
        return;
    busy_     = false;
    received_ = direction_ == Receiving;
    service();
    if (direction_ == Sending) {
        if (tx_)
            tx_(txArg_);
        else if (txSimple_)
            txSimple_();
    } else {
        if (rx_)
            rx_(rxArg_);
        else if (rxSimple_)
            rxSimple_();
    }
}
void I2SClass::setBufferSize(int size)
{
    if (!ready_)
        buffer_.setSize(size);
}
void I2SClass::onTransmit(callback_with_arg_t cb, void* arg)
{
    InterruptGuard guard;
    tx_       = cb;
    txArg_    = arg;
    txSimple_ = nullptr;
}
void I2SClass::onReceive(callback_with_arg_t cb, void* arg)
{
    InterruptGuard guard;
    rx_       = cb;
    rxArg_    = arg;
    rxSimple_ = nullptr;
}
void I2SClass::onTransmit(callback_t cb)
{
    InterruptGuard guard;
    txSimple_ = cb;
    tx_       = nullptr;
}
void I2SClass::onReceive(callback_t cb)
{
    InterruptGuard guard;
    rxSimple_ = cb;
    rx_       = nullptr;
}
