/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-5-30      Bernard      the first version
 */

#ifndef BOARD_H__
#define BOARD_H__

#include <rtconfig.h>

extern unsigned int __bss_start;
extern unsigned int __bss_end;

#define RT_HW_HEAP_BEGIN    (void*)&__bss_end
#define RT_HW_HEAP_END      (void*)(0x80000000 + 6 * 1024 * 1024)

#if ENABLE_CONSOLE
void rt_hw_console_output(const char *str);
#endif /* ENABLE_CONSOLE */

void primary_cpu_entry(void);

void rt_hw_board_init(void);

void rt_hw_cpu_reset(void);

void rt_hw_us_delay(rt_uint32_t usec);

#endif /* BOARD_H__ */
