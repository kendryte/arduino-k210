// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
// K210 has a unified address space. Read fixed-width values without alignment assumptions.
typedef void          prog_void;
typedef char          prog_char;
typedef unsigned char prog_uchar;
typedef int8_t        prog_int8_t;
typedef uint8_t       prog_uint8_t;
typedef int16_t       prog_int16_t;
typedef uint16_t      prog_uint16_t;
typedef int32_t       prog_int32_t;
typedef uint32_t      prog_uint32_t;
#define PROGMEM
#define PGM_P                       const char*
#define PGM_VOID_P                  const void*
#define PSTR(text)                  (text)
#define _SFR_BYTE(value)            (value)
#define pgm_get_far_address(object) ((uintptr_t) & (object))
static inline uint8_t k210_pgm_byte(const void* address)
{
    uint8_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}
#define pgm_read_byte(address) k210_pgm_byte((const void*)(address))
static inline uint16_t k210_pgm_word(const void* address)
{
    uint16_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}
#define pgm_read_word(address) k210_pgm_word((const void*)(address))
static inline uint32_t k210_pgm_dword(const void* address)
{
    uint32_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}
#define pgm_read_dword(address) k210_pgm_dword((const void*)(address))
static inline float k210_pgm_float(const void* address)
{
    float value;
    memcpy(&value, address, sizeof(value));
    return value;
}
#define pgm_read_float(address) k210_pgm_float((const void*)(address))
static inline void* k210_pgm_ptr(const void* address)
{
    void* value;
    memcpy(&value, address, sizeof(value));
    return value;
}
#define pgm_read_ptr(address)        k210_pgm_ptr((const void*)(address))
#define pgm_read_byte_near(address)  pgm_read_byte(address)
#define pgm_read_byte_far(address)   pgm_read_byte(address)
#define pgm_read_word_near(address)  pgm_read_word(address)
#define pgm_read_word_far(address)   pgm_read_word(address)
#define pgm_read_dword_near(address) pgm_read_dword(address)
#define pgm_read_dword_far(address)  pgm_read_dword(address)
#define pgm_read_float_near(address) pgm_read_float(address)
#define pgm_read_float_far(address)  pgm_read_float(address)
#define pgm_read_ptr_near(address)   pgm_read_ptr(address)
#define pgm_read_ptr_far(address)    pgm_read_ptr(address)
#define memcmp_P                     memcmp
#define memccpy_P                    memccpy
#define memmem_P                     memmem
#define memcpy_P                     memcpy
#define strcpy_P                     strcpy
#define strncpy_P                    strncpy
#define strcat_P                     strcat
#define strncat_P                    strncat
#define strcmp_P                     strcmp
#define strncmp_P                    strncmp
#define strcasecmp_P                 strcasecmp
#define strncasecmp_P                strncasecmp
#define strlen_P                     strlen
#define strnlen_P                    strnlen
#define strstr_P                     strstr
#define printf_P                     printf
#define sprintf_P                    sprintf
#define snprintf_P                   snprintf
#define vsnprintf_P                  vsnprintf
