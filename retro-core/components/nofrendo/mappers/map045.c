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
** map045.c: Mapper 45 (Super 1M-999999-in-1) interface
**
*/

#include "nes/nes.h"

static uint8 regs[4];
static uint8 write_count;


static void map_write(uint32 address, uint8 value)
{
    if (address < 0x6000)
        return;

    if (address < 0x8000)
    {
        if (write_count < 4)
        {
            regs[write_count++] = value;
        }
        return;
    }

    uint8 prg_bank = (regs[1] & 0x07) | ((regs[2] & 0x06) << 2);
    uint8 chr_bank = (regs[0] & 0x03) | ((regs[2] & 0x01) << 2) | ((regs[3] & 0x03) << 3);

    mmc_bankrom(16, 0x8000, prg_bank);
    mmc_bankvrom(8, 0x0000, chr_bank);
}

static void map_init(rom_t *cart)
{
    regs[0] = regs[1] = regs[2] = regs[3] = 0;
    write_count = 0;
    mmc_bankrom(16, 0x8000, 0);
    mmc_bankrom(16, 0xC000, MMC_LASTBANK);
}


mapintf_t map45_intf =
{
    .number     = 45,
    .name       = "Super 1M",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = NULL,
    .set_state  = NULL,
    .mem_read   = {},
    .mem_write  = {
        { 0x6000, 0xFFFF, map_write }
    },
};
