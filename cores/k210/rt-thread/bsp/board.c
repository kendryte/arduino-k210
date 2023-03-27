/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 *2021-06-10      xiaoyu       implement rt_hw_us_delay()
 */

#include <stdio.h>

#include "rthw.h"
#include "rtthread.h"
#include "rtdevice.h"

#include "board.h"
#include "tick.h"

#include "k210_bsp.h"
#include "k210-hal.h"

#ifndef F_CPU
#define F_CPU   400000000UL
#endif /* !F_CPU */

#define CONSOLE_USE_UARTHS   (0)

#if ENABLE_CONSOLE

static _lock_t _console_lock;

#if CONSOLE_USE_UARTHS

static volatile uarths_t *const _uarths = (volatile uarths_t *)UARTHS_BASE_ADDR;

static inline char _uarths_putchar(char c)
{
    while(_uarths->txdata.full)
        continue;
    _uarths->txdata.data = (uint8_t)c;

    return (c & 0xff);
}

static void _uarths_init(uint32_t baud)
{
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    uint16_t div = freq / baud - 1;

    /* Set UART registers */
    _uarths->div.div = div;
    _uarths->txctrl.nstop = UARTHS_STOP_1;

    _uarths->txctrl.txen = 1;
    _uarths->rxctrl.rxen = 1;
    _uarths->txctrl.txcnt = 0;
    _uarths->rxctrl.rxcnt = 0;
    _uarths->ip.txwm = 1;
    _uarths->ip.rxwm = 1;
    _uarths->ie.txwm = 0;
    _uarths->ie.rxwm = 1;
}

static void _init_console_uart(void)
{
    _lock_init(&_console_lock);

    hal_fpioa_set_pin_func(4, FUNC_UARTHS_RX);
    hal_fpioa_set_pin_func(5, FUNC_UARTHS_TX);

    _uarths_init(1500000);
}

#else

extern char _SerialPutChar(char c);

static inline char _uarths_putchar(char c)
{
    return _SerialPutChar(c);
}

static void _init_console_uart(void)
{
    _lock_init(&_console_lock);
}
#endif // CONSOLE_USE_UARTHS

void rt_hw_console_output(const char *str)
{
    _lock_acquire(&_console_lock);

    while(*str) {
        if(_uarths_putchar(*str++) == 0) {
            _lock_release(& _console_lock);
            return;
        }
    }
    _lock_release(& _console_lock);
}

int rt_hw_console_write(const void *buf, size_t size)
{
    const char *data = (const char*)buf;

    for(size_t i = 0; i < size; i++) {
        _uarths_putchar(data[i]);
    }

    return 0;
}
#else
int rt_hw_console_write(const void *buf, size_t size)
{
    return 0;
}
#endif /* ENABLE_CONSOLE */

static void init_bss(void)
{
    unsigned int *dst;

    dst = &__bss_start;
    while (dst < &__bss_end)
    {
        *dst++ = 0;
    }
}

void primary_cpu_entry(void)
{
    /* disable global interrupt */
    init_bss();
    rt_hw_interrupt_disable();

    extern void entry(void);
    entry();
}

void rt_hw_board_init(void)
{
    sysctl_pll_set_freq(SYSCTL_PLL0, F_CPU * 2);
    sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_APB1, 2);

    /* Init FPIOA */
    fpioa_init();
    hal_fpioa_init();
    /* Dmac init */
    dmac_init();

#if ENABLE_CONSOLE
    _init_console_uart();
#endif /* ENABLE_CONSOLE */

    /* initalize interrupt */
    rt_hw_interrupt_init();
    /* initialize hardware interrupt */

    rt_hw_tick_init();

#ifdef RT_USING_SMP
    extern int rt_hw_clint_ipi_enable(void);
    rt_hw_clint_ipi_enable();
#endif

#ifdef RT_USING_HEAP
    rt_kprintf("heap: [0x%08x - 0x%08x]\n", (rt_ubase_t) RT_HW_HEAP_BEGIN , (rt_ubase_t) RT_HW_HEAP_END );
    /* initialize memory system */
    rt_system_heap_init( RT_HW_HEAP_BEGIN, RT_HW_HEAP_END );
#endif

    extern int cplusplus_system_init(void);
    cplusplus_system_init();

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

void rt_hw_cpu_reset(void)
{
    sysctl->soft_reset.soft_reset = 1;
    while(1);
}

/**
 * This function will delay for some us.
 *
 * @param us the delay time of us
 */
void rt_hw_us_delay(rt_uint32_t usec)
{
    uint64_t cycle = read_cycle();
    uint64_t nop_all = (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL) * usec;

    while (1) {
       if(read_cycle() - cycle >= nop_all)
            break;
    }
}

void plic_irq_handle(plic_irq_t irq)
{
    plic_instance_t (*plic_instance)[IRQN_MAX] = plic_get_instance();
    if (plic_instance[0][irq].callback)
    {
        plic_instance[0][irq].callback(
            plic_instance[0][irq].ctx);
    }
    else if (plic_instance[1][irq].callback)
    {
        plic_instance[1][irq].callback(
            plic_instance[1][irq].ctx);
    }
}
