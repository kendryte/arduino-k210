#pragma once

#include <string>
#include <stdint.h>

#include "rtthread.h"
#include "k210-hal.h"

class ScopedTimer
{
public:
    // The constructor starts the timer immediately.
    // It takes a label to identify the code block being timed.
    ScopedTimer(std::string label, bool debug = true) :
        m_label(std::move(label)),
        m_startTime(sysctl_get_time_us()),
        m_debug(debug)
    {
    }

    // The destructor is called automatically when the object goes out of scope.
    // It stops the timer, calculates the duration, and reports the result.
    ~ScopedTimer()
    {
        // 1. Stop the clock
        auto endTime = sysctl_get_time_us();

        // 2. Calculate duration in microseconds (μs) for precision
        auto duration = endTime - m_startTime;
        // 3. Report the result
        if (m_debug) {
            rt_kprintf("[%s] finished in %llu us.\n", m_label.c_str(), duration);
        }
    }

    // Disable copying and assignment to ensure the timer is tied only to its scope
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
private:
    std::string m_label;
    uint64_t m_startTime;
    bool m_debug;
};

// Optional: A shorter macro for common usage
#define SCOPE_TIMER(label) ScopedTimer timer_##label(#label)
