// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "Wire.h"
#include <cstring>

namespace {
TwoWire* owners[I2C_DEVICE_MAX] = {};
class Lock {
public:
    explicit Lock(rt_mutex_t mutex)
        : mutex_(mutex)
    {
        if (mutex_ && rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK)
            mutex_ = nullptr;
    }
    ~Lock()
    {
        if (mutex_)
            rt_mutex_release(mutex_);
    }
    explicit operator bool() const
    {
        return mutex_ != nullptr;
    }

private:
    rt_mutex_t mutex_;
};
}
TwoWire Wire(0);
TwoWire::TwoWire(int busNum)
    : _i2cNum(static_cast<i2c_device_number_t>(busNum >= 0 && busNum < I2C_DEVICE_MAX ? busNum : I2C_DEVICE_MAX))
{
}
TwoWire::~TwoWire()
{
    end();
}

bool TwoWire::begin(int8_t sclPin, int8_t sdaPin, uint32_t frequency)
{
    if (_i2cNum == I2C_DEVICE_MAX || sclPin < 0 || sclPin >= K210_IO_NUMS || sdaPin < 0 || sdaPin >= K210_IO_NUMS
        || sclPin == sdaPin || !frequency)
        return false;
    if (ready_)
        return sclPin == scl_ && sdaPin == sda_ && frequency == clock_;
    TwoWire* empty = nullptr;
    if (!__atomic_compare_exchange_n(&owners[_i2cNum], &empty, this, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        return false;
    mutex_ = rt_mutex_create("wire", RT_IPC_FLAG_PRIO);
    if (!mutex_) {
        __atomic_store_n(&owners[_i2cNum], static_cast<TwoWire*>(nullptr), __ATOMIC_RELEASE);
        return false;
    }
    clock_ = frequency;
    scl_   = sclPin;
    sda_   = sdaPin;
    hal_fpioa_set_pin_func(scl_, static_cast<fpioa_function_t>(FUNC_I2C0_SCLK + 2 * _i2cNum));
    hal_fpioa_set_pin_func(sda_, static_cast<fpioa_function_t>(FUNC_I2C0_SDA + 2 * _i2cNum));
    i2c_init(_i2cNum, 0, 7, clock_);
    ready_ = true;
    clearWriteError();
    return true;
}
bool TwoWire::begin(uint16_t, int8_t, int8_t, uint32_t)
{
    return false;
}
bool TwoWire::end()
{
    if (!ready_)
        return true;
    // Lifecycle operations require the application to stop all users first.
    i2c[_i2cNum]->enable = 0;
    hal_fpioa_clr_pin_func(scl_);
    hal_fpioa_clr_pin_func(sda_);
    cancel();
    rxSize_ = rxOffset_ = 0;
    ready_              = false;
    rt_mutex_delete(mutex_);
    mutex_ = nullptr;
    __atomic_store_n(&owners[_i2cNum], static_cast<TwoWire*>(nullptr), __ATOMIC_RELEASE);
    return true;
}
bool TwoWire::accessible() const
{
    return phase_ == Idle || task_ == rt_thread_self();
}
void TwoWire::cancel()
{
    phase_    = Idle;
    task_     = nullptr;
    txSize_   = 0;
    overflow_ = false;
}
void TwoWire::setClock(uint32_t frequency)
{
    Lock lock(mutex_);
    if (lock && accessible() && frequency)
        clock_ = frequency;
}
void TwoWire::start(uint16_t address)
{
    Lock lock(mutex_);
    if (!lock || !accessible())
        return;
    cancel();
    clearWriteError();
    address_ = address;
    task_    = rt_thread_self();
    phase_   = Collecting;
    if (address > 1023)
        setWriteError();
}
size_t TwoWire::write(const uint8_t* buffer, size_t count)
{
    Lock lock(mutex_);
    if (!lock || !accessible())
        return 0;
    if (phase_ != Collecting || address_ > 1023 || (!buffer && count)) {
        setWriteError();
        return 0;
    }
    const size_t room     = sizeof(tx_) - txSize_;
    const size_t accepted = count < room ? count : room;
    if (accepted)
        std::memcpy(tx_ + txSize_, buffer, accepted);
    txSize_ += accepted;
    if (accepted != count) {
        overflow_ = true;
        setWriteError();
    }
    return accepted;
}
uint8_t TwoWire::transferStatus(int sdkResult) const
{
    if (!sdkResult)
        return 0;
    const uint32_t cause = i2c[_i2cNum]->tx_abrt_source;
    if (cause & (I2C_TX_ABRT_SOURCE_7B_ADDR_NOACK | I2C_TX_ABRT_SOURCE_10B_ADDR1_NOACK | I2C_TX_ABRT_SOURCE_10B_ADDR2_NOACK))
        return 2;
    if (cause & I2C_TX_ABRT_SOURCE_TXDATA_NOACK)
        return 3;
    return 4;
}
uint8_t TwoWire::endTransmission(bool stop)
{
    Lock lock(mutex_);
    if (!lock || !accessible())
        return 4;
    uint8_t status = 4;
    if (phase_ == Collecting && address_ <= 1023) {
        if (overflow_)
            status = 1;
        else if (!getWriteError() && txSize_) {
            if (!stop) {
                phase_ = CommandPending;
                return 0;
            }
            i2c_init(_i2cNum, address_, address_ > 127 ? 10 : 7, clock_);
            status = transferStatus(i2c_send_data(_i2cNum, tx_, txSize_));
        }
    }
    // The SDK cannot issue an address-only probe; never report a false ACK.
    cancel();
    if (status)
        setWriteError();
    return status;
}
size_t TwoWire::receive(uint16_t address, size_t count, bool stop)
{
    Lock lock(mutex_);
    if (!lock || !accessible())
        return 0;
    rxSize_ = rxOffset_ = 0;
    const bool valid    = stop && address <= 1023 && phase_ != Collecting && (phase_ != CommandPending || address == address_);
    if (!valid || !count) {
        if (!valid)
            setWriteError();
        cancel();
        return 0;
    }
    if (count > sizeof(rx_))
        count = sizeof(rx_);
    i2c_init(_i2cNum, address, address > 127 ? 10 : 7, clock_);
    // SDK receive does not clear an abort left by the preceding transaction.
    (void)i2c[_i2cNum]->clr_tx_abrt;
    // A deferred command and its read are submitted in one SDK operation.
    const int result = i2c_recv_data(_i2cNum, tx_, txSize_, rx_, count);
    cancel();
    if (!result)
        rxSize_ = count;
    else
        setWriteError();
    return rxSize_;
}
uint8_t TwoWire::requestFrom(int address, int count, int stop)
{
    return receive(address < 0 || address > 1023 ? 1024 : address, count > 0 ? static_cast<size_t>(count) : 0, stop != 0);
}
int TwoWire::available()
{
    Lock lock(mutex_);
    return lock && accessible() ? static_cast<int>(rxSize_ - rxOffset_) : 0;
}
int TwoWire::peek()
{
    Lock lock(mutex_);
    return lock && accessible() && rxOffset_ < rxSize_ ? rx_[rxOffset_] : -1;
}
int TwoWire::read()
{
    Lock lock(mutex_);
    return lock && accessible() && rxOffset_ < rxSize_ ? rx_[rxOffset_++] : -1;
}
void TwoWire::flush()
{
    Lock lock(mutex_);
    if (lock && accessible()) {
        cancel();
        rxSize_ = rxOffset_ = 0;
    }
}
