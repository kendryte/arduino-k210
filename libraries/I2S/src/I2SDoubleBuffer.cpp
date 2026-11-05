// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "I2SDoubleBuffer.h"
#include "rtthread.h"
#include <limits.h>
#include <string.h>
void I2SDoubleBuffer::setSize(int size)
{
    if (!storage_ && size >= 4 && size <= INT_MAX / 2)
        capacity_ = size - size % 4;
}
void I2SDoubleBuffer::reset()
{
    if (!storage_)
        storage_ = static_cast<uint8_t*>(rt_malloc_align(static_cast<size_t>(capacity_) * 2, 8));
    application_ = 0;
    flush();
}
void I2SDoubleBuffer::end()
{
    if (storage_)
        rt_free_align(storage_);
    storage_ = nullptr;
    flush();
}
size_t I2SDoubleBuffer::availableForWrite()
{
    return storage_ ? capacity_ - used_ : 0;
}
size_t I2SDoubleBuffer::available()
{
    return storage_ ? used_ - consumed_ : 0;
}
size_t I2SDoubleBuffer::write(const void* buffer, size_t size)
{
    if (!buffer)
        return 0;
    if (size > availableForWrite())
        size = availableForWrite();
    if (size)
        memcpy(storage_ + application_ * capacity_ + used_, buffer, size);
    used_ += size;
    return size;
}
size_t I2SDoubleBuffer::peek(void* buffer, size_t size)
{
    if (!buffer)
        return 0;
    if (size > available())
        size = available();
    if (size)
        memcpy(buffer, storage_ + application_ * capacity_ + consumed_, size);
    return size;
}
size_t I2SDoubleBuffer::read(void* buffer, size_t size)
{
    const size_t count = peek(buffer, size);
    consumed_ += count;
    return count;
}
void* I2SDoubleBuffer::data()
{
    return storage_ ? storage_ + (1 - application_) * capacity_ : nullptr;
}
void I2SDoubleBuffer::swap(int length)
{
    if (!storage_)
        return;
    application_ = 1 - application_;
    consumed_    = 0;
    used_        = length > 0 ? static_cast<size_t>(length > capacity_ ? capacity_ : length) : 0;
}
void I2SDoubleBuffer::flush()
{
    used_ = consumed_ = 0;
}
