/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef COMMON_H
#define COMMON_H

#include <sdkconfig.h>

#define ror(dest, value, shift)                                               \
  dest = ((value) >> (shift)) | ((value) << (32 - (shift)))                   \

#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define MIN(a,b)  ((a) < (b) ? (a) : (b))

#ifdef CONFIG_IDF_TARGET_ESP32P4
  #ifndef likely
    #define likely(x)   __builtin_expect(!!(x), 1)
  #endif
  #ifndef unlikely
    #define unlikely(x) __builtin_expect(!!(x), 0)
  #endif
  #define GBA_CACHE_LINE_SIZE      64
  #define GBA_CACHE_ALIGN          __attribute__((aligned(GBA_CACHE_LINE_SIZE)))
  #define GBA_ALWAYS_INLINE        __attribute__((always_inline)) inline
#else
  #ifndef likely
    #define likely(x)   (x)
  #endif
  #ifndef unlikely
    #define unlikely(x) (x)
  #endif
  #define GBA_CACHE_LINE_SIZE      1
  #define GBA_CACHE_ALIGN
  #define GBA_ALWAYS_INLINE        inline
#endif

#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_CHAR '/'

#define function_cc

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long int u64;
typedef signed long long int s64;

#if defined(USE_XBGR1555_FORMAT)
  #define convert_palette(value)  \
    (value & 0x7FFF)
#else
  #define convert_palette(value) \
    (((value & 0x1F) << 11) | ((value & 0x03E0) << 1) | ((value >> 10) & 0x1F))
#endif

#define GBA_SCREEN_WIDTH  (240)
#define GBA_SCREEN_HEIGHT (160)
#define GBA_SCREEN_PITCH  (240)

#define GBA_SCREEN_BUFFER_SIZE  \
  (GBA_SCREEN_PITCH * (GBA_SCREEN_HEIGHT + 1) * sizeof(uint16_t))

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define netorder32(value) (value)
#else
  #define netorder32(value) __builtin_bswap32(value)
#endif

typedef u32 fixed16_16;
typedef u32 fixed8_24;

#define float_to_fp16_16(value)                                               \
  (fixed16_16)((value) * 65536.0)                                             \

#define fp16_16_to_float(value)                                               \
  (float)((value) / 65536.0)                                                  \

#define u32_to_fp16_16(value)                                                 \
  ((value) << 16)                                                             \

#define fp16_16_to_u32(value)                                                 \
  ((value) >> 16)                                                             \

#define fp16_16_fractional_part(value)                                        \
  ((value) & 0xFFFF)                                                          \

#define float_to_fp8_24(value)                                                \
  (fixed8_24)((value) * 16777216.0)                                           \

#define fp8_24_fractional_part(value)                                         \
  ((value) & 0xFFFFFF)                                                        \

#define fixed_div(numerator, denominator, bits)                               \
  (((numerator * (1 << bits)) + (denominator / 2)) / denominator)             \

#define address8(base, offset)                                                \
  *((u8 *)((u8 *)base + (offset)))                                            \

#define address16(base, offset)                                               \
  *((u16 *)((u8 *)base + (offset)))                                           \

#define address32(base, offset)                                               \
  *((u32 *)((u8 *)base + (offset)))                                           \

#define eswap8(value)  (value)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define eswap16(value) __builtin_bswap16(value)
  #define eswap32(value) __builtin_bswap32(value)
#else
  #define eswap16(value) (value)
  #define eswap32(value) (value)
#endif

#define  readaddress8(base, offset) eswap8( address8( base, offset))
#define readaddress16(base, offset) eswap16(address16(base, offset))
#define readaddress32(base, offset) eswap32(address32(base, offset))

#define read_ioreg(regnum) (eswap16(io_registers[(regnum)]))
#define write_ioreg(regnum, val) io_registers[(regnum)] = eswap16(val)
#define read_ioreg32(regnum) (read_ioreg(regnum) | (read_ioreg((regnum)+1) << 16))

#define read_dmareg(regnum, dmachan) (eswap16(io_registers[(regnum) + (dmachan) * 6]))
#define write_dmareg(regnum, dmachan, val) io_registers[(regnum) + (dmachan) * 6] = eswap16(val)

#include <rg_system.h>
#include <esp_attr.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cpu.h"
#include "gba_memory.h"
#include "savestate.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "main.h"
#include "cheats.h"
#include "serial.h"

#endif
