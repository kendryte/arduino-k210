#pragma once

#include "k210_atomic.h"

typedef long _lock_t;

static inline long lock_trylock(_lock_t *lock)
{
    long res = atomic_swap(lock, -1);
    /* Use memory barrier to keep coherency */
    mb();
    return res;
}

static inline void lock_lock(_lock_t *lock)
{
    while(lock_trylock(lock))
        ;
}

static inline void lock_unlock(_lock_t *lock)
{
    /* Use memory barrier to keep coherency */
    mb();
    atomic_swap(lock, 0);
    asm volatile("nop");
}

static inline void _lock_init(_lock_t *lock)
{
    *lock = 0;
}

// static inline void _lock_init_recursive(_lock_t *lock)
// {
//     reculock_init(lock);
// }

static inline void _lock_close(_lock_t *lock)
{
    lock_unlock(lock);
}

// static inline void _lock_close_recursive(_lock_t *lock)
// {
//     reculock_deinit(lock);
// }

static inline void _lock_acquire(_lock_t *lock)
{
    lock_lock(lock);
}

// static inline void _lock_acquire_recursive(_lock_t *lock)
// {
//     reculock_lock(lock);
// }

static inline int _lock_try_acquire(_lock_t *lock)
{
    return lock_trylock(lock);
}

// static inline int _lock_try_acquire_recursive(_lock_t *lock)
// {
//     return reculock_trylock(lock);
// }

static inline void _lock_release(_lock_t *lock)
{
    lock_unlock(lock);
}

// static inline void _lock_release_recursive(_lock_t *lock)
// {
//     reculock_unlock(lock);
// }
