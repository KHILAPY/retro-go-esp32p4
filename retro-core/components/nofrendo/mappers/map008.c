/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
** map008.c: FFE F3xxx mapper interface
**
** FFE Copier Mapper - F3xxx variant
** Used by: Doraemon Kaitakuhen [hacked], Paris Dakar Rally [Hacked]
*/

#include "nes/nes.h"

static struct
{
    uint8 latch;
    uint8 mirr;
    bool irq_enabled;
    int32 irq_counter;
} ffe8;

static void sync(void)
{
    mmc_bankchr(8, 0x0000, ffe8.latch & 0x03, CHR_ANY);
    mmc_bankrom(16, 0x8000, (ffe8.latch >> 2) & 0x3F);
    mmc_bankrom(16, 0xC000, MMC_LASTBANK);
    
    switch (ffe8.mirr)
    {
    case 0: ppu_setmirroring(PPU_MIRROR_SCR0); break;
    case 1: ppu_setmirroring(PPU_MIRROR_SCR1); break;
    case 2: ppu_setmirroring(PPU_MIRROR_VERT); break;
    case 3: ppu_setmirroring(PPU_MIRROR_HORI); break;
    }
}

static void map_write_latch(uint32 address, uint8 value)
{
    ffe8.latch = value;
    sync();
}

static void map_write_mirr(uint32 address, uint8 value)
{
    ffe8.mirr = ((address << 1) & 2) | ((value >> 4) & 1);
    sync();
}

static void map_write_irq(uint32 address, uint8 value)
{
    switch (address)
    {
    case 0x4501:
        ffe8.irq_enabled = false;
        break;
    case 0x4502:
        ffe8.irq_counter = (ffe8.irq_counter & 0xFF00) | value;
        break;
    case 0x4503:
        ffe8.irq_counter = (ffe8.irq_counter & 0x00FF) | (value << 8);
        ffe8.irq_enabled = true;
        break;
    }
}

static void map_hblank(nes_t *nes)
{
    if (nes->scanline > 240)
        return;

    if (!ppu_enabled())
        return;

    if (!ffe8.irq_enabled)
        return;
    
    ffe8.irq_counter += 114;
    if (ffe8.irq_counter >= 0x10000)
    {
        ffe8.irq_enabled = false;
        ffe8.irq_counter = 0;
        nes6502_irq();
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = ffe8.latch;
    state[1] = ffe8.mirr;
    state[2] = ffe8.irq_enabled;
    state[3] = ffe8.irq_counter & 0xFF;
    state[4] = (ffe8.irq_counter >> 8) & 0xFF;
    state[5] = (ffe8.irq_counter >> 16) & 0xFF;
}

static void map_setstate(uint8 *state)
{
    ffe8.latch = state[0];
    ffe8.mirr = state[1];
    ffe8.irq_enabled = state[2];
    ffe8.irq_counter = state[3] | (state[4] << 8) | (state[5] << 16);
    sync();
}

static void map_init(rom_t *cart)
{
    ffe8.latch = 0;
    ffe8.mirr = 2;
    ffe8.irq_enabled = false;
    ffe8.irq_counter = 0;
    
    mmc_bankprg(8, 0x6000, 0, PRG_RAM);
    sync();
}

mapintf_t map8_intf =
{
    .number     = 8,
    .name       = "FFE F3xxx",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = map_hblank,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x42FE, 0x42FF, map_write_mirr },
        { 0x4500, 0x4503, map_write_irq },
        { 0x8000, 0xFFFF, map_write_latch }
    },
};
