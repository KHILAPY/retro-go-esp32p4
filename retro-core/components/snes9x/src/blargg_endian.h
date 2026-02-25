/* This file is part of Snes9x. See LICENSE file. */

/* CPU Byte Order Utilities - ESP32P4 专用 */

#ifndef BLARGG_ENDIAN
#define BLARGG_ENDIAN

#include <stdbool.h>
#include <stdint.h>

/* ESP32P4 是小端序架构，直接使用指针访问 */
#define GET_LE16( addr )        (*(uint16_t*) (addr))
#define GET_LE32( addr )        (*(uint32_t*) (addr))
#define SET_LE16( addr, data )  (void) (*(uint16_t*) (addr) = (data))
#define SET_LE32( addr, data )  (void) (*(uint32_t*) (addr) = (data))

#define GET_LE16SA( addr )      ((int16_t) GET_LE16( addr ))
#define GET_LE16A( addr )       GET_LE16( addr )
#define SET_LE16A( addr, data ) SET_LE16( addr, data )

#endif
