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
** map114.c: Mapper 114 interface
** Used by: Lion King (Chinese)
**
*/

#include "nes/nes.h"

static uint8 command;
static uint8 prg_mode;
static uint8 chr_mode;


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xE001)
    {
    case 0x8000:
        command = value & 0x07;
        prg_mode = (value >> 6) & 0x03;
        chr_mode = (value >> 7) & 0x01;
        break;

    case 0x8001:
        switch (command)
        {
        case 0:
            mmc_bankvrom(1, 0x0000, value);
            break;
        case 1:
            mmc_bankvrom(1, 0x0400, value);
            break;
        case 2:
            mmc_bankvrom(1, 0x0800, value);
            break;
        case 3:
            mmc_bankvrom(1, 0x0C00, value);
            break;
        case 4:
            mmc_bankvrom(1, 0x1000, value);
            break;
        case 5:
            mmc_bankvrom(1, 0x1400, value);
            break;
        case 6:
            mmc_bankrom(8, 0x8000, value);
            break;
        case 7:
            mmc_bankrom(8, 0xC000, value);
            break;
        }
        break;
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = command;
    state[1] = prg_mode;
    state[2] = chr_mode;
}

static void map_setstate(uint8 *state)
{
    command = state[0];
    prg_mode = state[1];
    chr_mode = state[2];
}

static void map_init(rom_t *cart)
{
    command = 0;
    prg_mode = 0;
    chr_mode = 0;

    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 0);
    mmc_bankrom(8, 0xC000, 0);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
}


mapintf_t map114_intf =
{
    .number     = 114,
    .name       = "Mapper 114",
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
