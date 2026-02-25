/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
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
**
** map211.c: Mapper 211 interface
** Used by: Bugs Bunny Crazy Castle 2
**
*/

#include "nes/nes.h"

static uint8 prg_bank;
static uint8 chr_bank;


static void map_write(uint32 address, uint8 value)
{
    if (address & 0x4000)
    {
        prg_bank = value & 0x07;
        mmc_bankrom(32, 0x8000, prg_bank);
    }
    else
    {
        chr_bank = value & 0x0F;
        mmc_bankvrom(8, 0x0000, chr_bank);
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = prg_bank;
    state[1] = chr_bank;
}

static void map_setstate(uint8 *state)
{
    prg_bank = state[0];
    chr_bank = state[1];
    mmc_bankrom(32, 0x8000, prg_bank);
    mmc_bankvrom(8, 0x0000, chr_bank);
}

static void map_init(rom_t *cart)
{
    prg_bank = 0;
    chr_bank = 0;

    mmc_bankrom(32, 0x8000, 0);
    mmc_bankvrom(8, 0x0000, 0);
}


mapintf_t map211_intf =
{
    .number     = 211,
    .name       = "Mapper 211",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x8000, 0xFFFF, map_write }
    },
};
