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
** map068.c: Mapper 68 (Sunsoft 4) interface
** Used by: After Burner 2, Maharaja
**
*/

#include "nes/nes.h"

static uint8 reg[4];
static uint8 mirroring;


static void update_mirroring(void)
{
    switch (mirroring)
    {
    case 0:
        ppu_setmirroring(PPU_MIRROR_SCR0);
        break;
    case 1:
        ppu_setmirroring(PPU_MIRROR_SCR1);
        break;
    case 2:
        ppu_setmirroring(PPU_MIRROR_VERT);
        break;
    case 3:
        ppu_setmirroring(PPU_MIRROR_HORI);
        break;
    }
}

static void map_write(uint32 address, uint8 value)
{
    int reg_num = (address >> 12) & 0x03;

    switch (address & 0xF000)
    {
    case 0x8000:
        mmc_bankvrom(2, 0x0000, value);
        break;

    case 0x9000:
        mmc_bankvrom(2, 0x0800, value);
        break;

    case 0xA000:
        mmc_bankvrom(2, 0x1000, value);
        break;

    case 0xB000:
        mmc_bankvrom(2, 0x1800, value);
        break;

    case 0xC000:
        mmc_bankrom(16, 0x8000, value);
        break;

    case 0xD000:
        mmc_bankrom(16, 0xC000, value);
        break;

    case 0xE000:
        reg[reg_num] = value;
        mirroring = ((reg[0] & 0x10) >> 1) |
                    ((reg[1] & 0x10) >> 2) |
                    ((reg[2] & 0x10) >> 3) |
                    ((reg[3] & 0x10) >> 4);
        update_mirroring();
        break;

    case 0xF000:
        reg[reg_num] = value;
        mirroring = ((reg[0] & 0x10) >> 1) |
                    ((reg[1] & 0x10) >> 2) |
                    ((reg[2] & 0x10) >> 3) |
                    ((reg[3] & 0x10) >> 4);
        update_mirroring();
        break;
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = reg[0];
    state[1] = reg[1];
    state[2] = reg[2];
    state[3] = reg[3];
    state[4] = mirroring;
}

static void map_setstate(uint8 *state)
{
    reg[0] = state[0];
    reg[1] = state[1];
    reg[2] = state[2];
    reg[3] = state[3];
    mirroring = state[4];
    update_mirroring();
}

static void map_init(rom_t *cart)
{
    reg[0] = reg[1] = reg[2] = reg[3] = 0;
    mirroring = 0;

    mmc_bankrom(16, 0x8000, 0);
    mmc_bankrom(16, 0xC000, MMC_LASTBANK);
    mmc_bankvrom(8, 0x0000, 0);
}


mapintf_t map68_intf =
{
    .number     = 68,
    .name       = "Sunsoft 4",
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
