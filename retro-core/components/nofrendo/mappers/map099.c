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
** map099.c: Mapper 99 (Vs. Unisystem) interface
** Used by: Vs. Castlevania, Vs. Ice Climber, Vs. Super Mario Bros
**
*/

#include "nes/nes.h"


static void map_write(uint32 address, uint8 value)
{
    if (address == 0x4016)
    {
        if (value & 0x04)
        {
            mmc_bankrom(32, 0x8000, 1);
        }
        else
        {
            mmc_bankrom(32, 0x8000, 0);
        }
    }
}

static uint8 map_read(uint32 address)
{
    return 0;
}

static void map_init(rom_t *cart)
{
    mmc_bankrom(32, 0x8000, 0);
}


mapintf_t map99_intf =
{
    .number     = 99,
    .name       = "Vs. Unisystem",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = NULL,
    .set_state  = NULL,
    .mem_read   = {
        { 0x4016, 0x4016, map_read }
    },
    .mem_write  = {
        { 0x4016, 0x4016, map_write }
    },
};
