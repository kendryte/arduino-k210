// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "SPI.h"
extern "C" uint8_t _sdcard_use_hw_spi1;
namespace {
SPIClass* owners[2] = {};
uint8_t   reversed(uint8_t input)
{
    uint8_t output = 0;
    for (unsigned bit = 0; bit < 8; ++bit) {
        output = (output << 1) | (input & 1);
        input >>= 1;
    }
    return output;
}
}
SPIClass SPI;
SPIClass::SPIClass(uint8_t bus)
    : bus_(bus)
{
}
SPIClass::~SPIClass()
{
    end();
}
void SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t cs)
{
    if (ready_ || bus_ > 1)
        return;
    // Explicit pins are recommended. With no pins, reuse existing FPIOA routing.
    int8_t pins[] = { sck, miso, mosi, cs };
    for (unsigned i = 0; i < 4; ++i) {
        if (pins[i] < -1 || pins[i] >= K210_IO_NUMS)
            return;
        for (unsigned j = 0; j < i; ++j)
            if (pins[i] >= 0 && pins[i] == pins[j])
                return;
    }
    if ((bus_ == 0 && g_spi0_in_use) || (bus_ == 1 && _sdcard_use_hw_spi1))
        return;
    SPIClass* expected = nullptr;
    if (!__atomic_compare_exchange_n(&owners[bus_], &expected, this, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        return;
    mutex_ = rt_mutex_create("spi", RT_IPC_FLAG_PRIO);
    if (!mutex_) {
        __atomic_store_n(&owners[bus_], static_cast<SPIClass*>(nullptr), __ATOMIC_RELEASE);
        return;
    }
    const int base    = bus_ ? FUNC_SPI1_D0 : FUNC_SPI0_D0;
    const int funcs[] = { base + 13, base + 1, base, base + 8 };
    for (unsigned i = 0; i < 4; ++i) {
        pins_[i] = pins[i];
        if (pins[i] >= 0)
            hal_fpioa_set_pin_func(pins[i], static_cast<fpioa_function_t>(funcs[i]));
    }
    ready_ = true;
    configure();
}
void SPIClass::end()
{
    if (!ready_)
        return;
    endTransaction();
    spi[bus_]->ser    = 0;
    spi[bus_]->ssienr = 0;
    for (int8_t& pin : pins_) {
        if (pin >= 0)
            hal_fpioa_clr_pin_func(pin);
        pin = -1;
    }
    ready_ = false;
    rt_mutex_delete(mutex_);
    mutex_ = nullptr;
    __atomic_store_n(&owners[bus_], static_cast<SPIClass*>(nullptr), __ATOMIC_RELEASE);
}
void SPIClass::configure()
{
    if (!ready_)
        return;
    spi_init(static_cast<spi_device_num_t>(bus_), static_cast<spi_work_mode_t>(settings_._dataMode), SPI_FF_STANDARD, 8, 0);
    spi_set_clk_rate(static_cast<spi_device_num_t>(bus_), settings_._clock);
}
void SPIClass::setBitOrder(uint8_t order)
{
    if (order > SPI_MSBFIRST || (mutex_ && rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK))
        return;
    settings_._bitOrder = order;
    if (mutex_)
        rt_mutex_release(mutex_);
}
void SPIClass::setDataMode(uint8_t mode)
{
    if (mode > SPI_MODE3 || (mutex_ && rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK))
        return;
    settings_._dataMode = mode;
    configure();
    if (mutex_)
        rt_mutex_release(mutex_);
}
void SPIClass::setFrequency(uint32_t hz)
{
    if (!hz || (mutex_ && rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK))
        return;
    settings_._clock = hz;
    configure();
    if (mutex_)
        rt_mutex_release(mutex_);
}
void SPIClass::beginTransaction(SPISettings settings)
{
    if (!ready_ || !settings._clock || settings._dataMode > SPI_MODE3 || settings._bitOrder > SPI_MSBFIRST)
        return;
    if (rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK)
        return;
    if (transaction_) {
        rt_mutex_release(mutex_);
        return;
    }
    transaction_ = true;
    settings_    = settings;
    configure();
}
void SPIClass::endTransaction()
{
    if (transaction_) {
        transaction_ = false;
        rt_mutex_release(mutex_);
    }
}
void SPIClass::transferBytes(const uint8_t* input, uint8_t* output, uint32_t size)
{
    if (!ready_ || !size)
        return;
    // RT-Thread mutexes are recursive: the transaction owner can transfer while
    // another task must wait until endTransaction releases the outer lock.
    if (rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK)
        return;
    auto* port   = spi[bus_];
    port->ssienr = 0;
    port->ctrlr0 &= ~(3U << 8); // SPI0/1: full duplex TMOD, eight-bit frames.
    port->ctrlr1  = 0;
    port->ssienr  = 1;
    port->ser     = 1; // Hardware chip select 0 stays asserted for the entire buffer.
    uint32_t sent = 0, received = 0;
    while (received < size) {
        // At most 32 outstanding frames: drain RX before its FIFO can overflow.
        while (sent < size && sent - received < 32 && port->txflr < 32) {
            const uint8_t value = input ? input[sent] : 0xff;
            port->dr[0]         = settings_._bitOrder == SPI_LSBFIRST ? reversed(value) : value;
            ++sent;
        }
        while (received < sent && port->rxflr) {
            const uint8_t value = port->dr[0];
            if (output)
                output[received] = settings_._bitOrder == SPI_LSBFIRST ? reversed(value) : value;
            ++received;
        }
    }
    while (port->sr & 1U) { } // Wait for the final wire bit before releasing CS.
    port->ser    = 0;
    port->ssienr = 0;
    rt_mutex_release(mutex_);
}
void SPIClass::transfer(void* buffer, uint32_t size)
{
    transferBytes(static_cast<uint8_t*>(buffer), static_cast<uint8_t*>(buffer), size);
}
uint8_t SPIClass::transfer(uint8_t value)
{
    uint8_t answer = 0;
    transferBytes(&value, &answer, 1);
    return answer;
}
uint32_t SPIClass::wordTransfer(uint32_t value, unsigned bytes)
{
    if (!ready_ || rt_mutex_take(mutex_, RT_WAITING_FOREVER) != RT_EOK)
        return 0;
    // Keep the byte order stable through packing, transfer and unpacking.
    uint8_t input[4], output[4] = {};
    for (unsigned i = 0; i < bytes; ++i)
        input[i] = value >> (8 * (settings_._bitOrder == SPI_MSBFIRST ? bytes - 1 - i : i));
    transferBytes(input, output, bytes);
    uint32_t result = 0;
    for (unsigned i = 0; i < bytes; ++i)
        result |= uint32_t(output[i]) << (8 * (settings_._bitOrder == SPI_MSBFIRST ? bytes - 1 - i : i));
    rt_mutex_release(mutex_);
    return result;
}
uint16_t SPIClass::transfer16(uint16_t value)
{
    return wordTransfer(value, 2);
}
uint32_t SPIClass::transfer32(uint32_t value)
{
    return wordTransfer(value, 4);
}
void SPIClass::write(uint8_t value)
{
    (void)transfer(value);
}
void SPIClass::write16(uint16_t value)
{
    (void)transfer16(value);
}
void SPIClass::write32(uint32_t value)
{
    (void)transfer32(value);
}
void SPIClass::writeBytes(const uint8_t* input, uint32_t size)
{
    if (input)
        transferBytes(input, nullptr, size);
}
