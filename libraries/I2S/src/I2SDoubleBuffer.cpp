/*
  Copyright (c) 2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdlib.h>
#include <string.h>

#include "I2SDoubleBuffer.h"

#include "rtthread.h"

I2SDoubleBuffer::I2SDoubleBuffer() :
  _size(DEFAULT_I2S_BUFFER_SIZE)
{
  _lock_init(&_lock);

  reset();
}

I2SDoubleBuffer::~I2SDoubleBuffer()
{
  _lock_acquire(&_lock);

  rt_free_align(_buff);

  _lock_release(&_lock);
}

void I2SDoubleBuffer::end()
{
  _lock_acquire(&_lock);

  rt_free_align(_buff);

  _index = 0;
  _length[0] = 0;
  _length[1] = 0;
  _readOffset[0] = 0;
  _readOffset[1] = 0;

  _lock_release(&_lock);
}

void I2SDoubleBuffer::setSize(int size)
{
  _lock_acquire(&_lock);

  _size = size;

  _lock_release(&_lock);
}

void I2SDoubleBuffer::reset()
{
  _lock_acquire(&_lock);

  if(_buff) {
    rt_free_align(_buff);
  }

  _buff = (uint8_t *)rt_malloc_align(_size * 2, 8);
  RT_ASSERT(_buff);

  _buffer[0] = _buff;
  _buffer[1] = (uint8_t*)(_buff + _size);

  _index = 0;
  _length[0] = 0;
  _length[1] = 0;
  _readOffset[0] = 0;
  _readOffset[1] = 0;

  _lock_release(&_lock);
}

size_t I2SDoubleBuffer::availableForWrite()
{
  _lock_acquire(&_lock);

  size_t r = (_size - (_length[_index] - _readOffset[_index]));

  _lock_release(&_lock);

  return r;
}

size_t I2SDoubleBuffer::write(const void *buffer, size_t size)
{
  size_t space = availableForWrite();

  if (size > space) {
    size = space;
  }

  if (size == 0) {
    return 0;
  }

  _lock_acquire(&_lock);

  rt_memcpy(&_buffer[_index][_length[_index]], buffer, size);

  _length[_index] += size;

  _lock_release(&_lock);

  return size;
}

size_t I2SDoubleBuffer::read(void *buffer, size_t size)
{
  size_t avail = available();

  if (size > avail) {
    size = avail;
  }

  if (size == 0) {
    return 0;
  }

  _lock_acquire(&_lock);

  rt_memcpy(buffer, &_buffer[_index][_readOffset[_index]], size);
  _readOffset[_index] += size;

  _lock_release(&_lock);

  return size;
}

size_t I2SDoubleBuffer::peek(void *buffer, size_t size)
{
  size_t avail = available();

  if (size > avail) {
    size = avail;
  }

  if (size == 0) {
    return 0;
  }

  _lock_acquire(&_lock);

  rt_memcpy(buffer, &_buffer[_index][_readOffset[_index]], size);

  _lock_release(&_lock);

  return size;
}

void* I2SDoubleBuffer::data()
{
  _lock_acquire(&_lock);

  void *p = (void*)_buffer[_index];

  _lock_release(&_lock);

  return p;
}

size_t I2SDoubleBuffer::available()
{
  _lock_acquire(&_lock);

  size_t r = _length[_index] - _readOffset[_index];

  _lock_release(&_lock);

  return r;
}

void I2SDoubleBuffer::swap(int length)
{
  _lock_acquire(&_lock);

  if (_index == 0) {
    _index = 1;
  } else {
    _index = 0;
  }

  _length[_index] = length;
  _readOffset[_index] = 0;

  _lock_release(&_lock);
}

void I2SDoubleBuffer::flush()
{
  _lock_acquire(&_lock);

  _index = 0;
  _length[0] = 0;
  _length[1] = 0;
  _readOffset[0] = 0;
  _readOffset[1] = 0;

  _lock_release(&_lock);
}
