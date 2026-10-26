// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "Ticker.h"
#include "rthw.h"
#include "rtthread.h"
#include <limits.h>
#include <string.h>

uint32_t Ticker::milliseconds(float seconds)
{
    uint32_t bits;
    memcpy(&bits, &seconds, sizeof(bits));
    if ((bits & 0x7f800000U) == 0x7f800000U || seconds <= 0)
        return 0;
    const double value = static_cast<double>(seconds) * 1000;
    return value >= UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(value);
}
void Ticker::schedule(uint32_t ms, bool repeat, std::function<void()> action)
{
    detach();
    if (!ms || !action)
        return;
    action_               = std::make_shared<std::function<void()>>(std::move(action));
    const rt_base_t level = rt_hw_interrupt_disable();
    _timer                = hal_timer_get_free_chn();
    if (_timer) {
        // timer_init resets every channel's callback; enable only this clock here.
        sysctl_clock_enable(static_cast<sysctl_clock_t>(SYSCTL_CLOCK_TIMER0 + _timer->dev.tim));
        timer_set_enable(_timer->dev.tim, _timer->chn.tim, 0);
        timer_set_interval(_timer->dev.tim, _timer->chn.tim, static_cast<size_t>(ms) * 1000000);
        if (timer_irq_register(_timer->dev.tim, _timer->chn.tim, !repeat, 1, fired, this) == 0)
            timer_set_enable(_timer->dev.tim, _timer->chn.tim, 1);
        else {
            hal_timer_release_chn(const_cast<timer_dev_chn_map_t*>(_timer));
            _timer = nullptr;
        }
    }
    rt_hw_interrupt_enable(level);
    if (!_timer)
        action_.reset();
}
void Ticker::detach()
{
    const rt_base_t level = rt_hw_interrupt_disable();
    if (_timer) {
        timer_set_enable(_timer->dev.tim, _timer->chn.tim, 0);
        timer_irq_unregister(_timer->dev.tim, _timer->chn.tim);
        hal_timer_release_chn(const_cast<timer_dev_chn_map_t*>(_timer));
        _timer = nullptr;
    }
    action_.reset();
    rt_hw_interrupt_enable(level);
}
bool Ticker::active()
{
    return _timer && timer_is_enable(_timer->dev.tim, _timer->chn.tim);
}
int Ticker::fired(void* context)
{
    auto* self = static_cast<Ticker*>(context);
    // Keep the callable alive if its callback detaches or reattaches this ticker.
    const auto action = self->action_;
    if (action && *action)
        (*action)();
    return 0;
}
void Ticker::_attach_ms(uint32_t ms, bool repeat, callback_with_arg_t cb, size_t arg)
{
    _callback = cb;
    _arg      = arg;
    if (cb)
        schedule(ms, repeat, [cb, arg] { cb(reinterpret_cast<void*>(arg)); });
    else
        detach();
}
