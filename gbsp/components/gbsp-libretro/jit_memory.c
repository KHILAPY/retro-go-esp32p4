/*
 * gameplaySP
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

/*
 * JIT Memory Access Wrappers
 * 
 * These functions provide a simplified interface for the JIT compiler
 * to access GBA memory. They wrap the existing memory access functions.
 */

#include "common.h"
#include "gba_memory.h"

uint32_t gba_mem_read32(uint32_t addr)
{
    return read_memory32(addr);
}

void gba_mem_write32(uint32_t addr, uint32_t value)
{
    write_memory32(addr, value);
}

uint16_t gba_mem_read16(uint32_t addr)
{
    return (uint16_t)read_memory16(addr);
}

void gba_mem_write16(uint32_t addr, uint16_t value)
{
    write_memory16(addr, value);
}

uint8_t gba_mem_read8(uint32_t addr)
{
    return (uint8_t)read_memory8(addr);
}

void gba_mem_write8(uint32_t addr, uint8_t value)
{
    write_memory8(addr, value);
}
