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
** map067.c: Mapper 67 (Sunsoft 3) interface
** Used by: After Burner, Fantasy Zone, Platoon
**
*/

#include "nes/nes.h"

static uint8 command;
static uint8 irq_counter;
static uint8 irq_enabled;


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xF800)
    {
    case 0x8800:
        mmc_bankvrom(2, 0x0000, value);
        break;

    case 0x9800:
        mmc_bankvrom(2, 0x0800, value);
        break;

    case 0xA800:
        mmc_bankvrom(2, 0x1000, value);
        break;

    case 0xB800:
        mmc_bankvrom(2, 0x1800, value);
        break;

    case 0xC800:
        if (command & 0x01)
        {
            irq_counter = (irq_counter & 0x00FF) | (value << 8);
        }
        else
        {
            irq_counter = (irq_counter & 0xFF00) | value;
        }
        break;

    case 0xD800:
        irq_enabled = value & 0x10;
        break;

    case 0xE800:
        command = value;
        mmc_bankrom(16, 0x8000, value & 0x07);
        if (value & 0x20)
            ppu_setmirroring(PPU_MIRROR_HORI);
        else
            ppu_setmirroring(PPU_MIRROR_VERT);
        break;
    }
}

static void map_hblank(nes_t *nes)
{
    if (nes->scanline > 240)
        return;

    if (!ppu_enabled())
        return;

    if (irq_enabled)
    {
        if (irq_counter > 0)
        {
            irq_counter--;
            if (irq_counter == 0)
            {
                irq_enabled = 0;
                nes6502_irq();
            }
        }
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = command;
    state[1] = irq_counter & 0xFF;
    state[2] = (irq_counter >> 8) & 0xFF;
    state[3] = irq_enabled;
}

static void map_setstate(uint8 *state)
{
    command = state[0];
    irq_counter = state[1] | (state[2] << 8);
    irq_enabled = state[3];
}

static void map_init(rom_t *cart)
{
    command = 0;
    irq_counter = 0;
    irq_enabled = 0;

    mmc_bankrom(16, 0x8000, 0);
    mmc_bankrom(16, 0xC000, MMC_LASTBANK);
    mmc_bankvrom(8, 0x0000, 0);
}


mapintf_t map67_intf =
{
    .number     = 67,
    .name       = "Sunsoft 3",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = map_hblank,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x8000, 0xFFFF, map_write }
    },
};
