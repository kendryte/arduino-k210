// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "k210-hal.h"
#include <functional>
#include <memory>

// Configure from a single task. Callbacks run in timer interrupt context.
class Ticker {
public:
    using callback_t          = void (*)();
    using callback_with_arg_t = void (*)(void*);
    Ticker()                  = default;
    ~Ticker()
    {
        detach();
    }
    Ticker(const Ticker&) = delete;
    Ticker& operator=(const Ticker&) = delete;
    void    attach(float seconds, callback_t cb)
    {
        attach_ms(milliseconds(seconds), cb);
    }
    void once(float seconds, callback_t cb)
    {
        once_ms(milliseconds(seconds), cb);
    }
    void attach_ms(uint32_t ms, callback_t cb)
    {
        schedule(ms, true, cb);
    }
    void once_ms(uint32_t ms, callback_t cb)
    {
        schedule(ms, false, cb);
    }
    template <class T> void attach(float s, void (*cb)(T), T arg)
    {
        attach_ms(milliseconds(s), cb, arg);
    }
    template <class T> void once(float s, void (*cb)(T), T arg)
    {
        once_ms(milliseconds(s), cb, arg);
    }
    template <class T> void attach_ms(uint32_t ms, void (*cb)(T), T arg)
    {
        if (cb)
            schedule(ms, true, [cb, arg] { cb(arg); });
        else
            detach();
    }
    template <class T> void once_ms(uint32_t ms, void (*cb)(T), T arg)
    {
        if (cb)
            schedule(ms, false, [cb, arg] { cb(arg); });
        else
            detach();
    }
    void detach();
    bool active();
    // Retained for source compatibility; typed overloads avoid function-pointer casts.
    size_t              _arg      = 0;
    callback_with_arg_t _callback = nullptr;

protected:
    void                       _attach_ms(uint32_t ms, bool repeat, callback_with_arg_t cb, size_t arg);
    const timer_dev_chn_map_t* _timer = nullptr;

private:
    std::shared_ptr<std::function<void()>> action_;
    static uint32_t                        milliseconds(float seconds);
    static int                             fired(void* context);
    void                                   schedule(uint32_t ms, bool repeat, std::function<void()> action);
};
