/* 
  Ticker.cpp - esp32 library that calls functions periodically

  Copyright (c) 2017 Bert Melis. All rights reserved.
  
  Based on the original work of:
  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
  The original version is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Ticker.h"

static int _timer_callback(void* ctx)
{
    auto &_ticker = *reinterpret_cast<Ticker *>(ctx);
    _ticker._callback((void *)_ticker._arg);
    return 0;
}

Ticker::Ticker()
{
  _timer = hal_timer_get_free_chn();

  if(NULL == _timer) {
    rt_kprintf("get free timer channel failed");
    return;
  }
  timer_init(_timer->dev.tim);
}

Ticker::~Ticker() {
  detach();
}

void Ticker::_attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, size_t arg) {
  _arg = arg;
  _callback = callback;

  timer_set_interval(_timer->dev.tim, _timer->chn.tim, milliseconds * 1000000);
  timer_irq_register(_timer->dev.tim, _timer->chn.tim, !repeat, 4, _timer_callback, this);
  timer_set_enable(_timer->dev.tim, _timer->chn.tim, 1);
}

void Ticker::detach() {
  if (_timer) {
    timer_irq_unregister(_timer->dev.tim, _timer->chn.tim);
    timer_set_enable(_timer->dev.tim, _timer->chn.tim, 0);
    _timer = nullptr;
  }
}

bool Ticker::active() {
  if (!_timer) return false;
  return timer_is_enable(_timer->dev.tim, _timer->chn.tim);
}
