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
** map069.c: Sunsoft FME-7 mapper interface
** Used by: Batman: Return of the Joker, Gimmick!
**
*/

#include "nes/nes.h"

static struct
{
    bool enabled;
    uint8 counter;
    uint8 latch;
} irq;

static uint8 command;


static void map_hblank(nes_t *nes)
{
    if (irq.enabled)
    {
        if (irq.counter == 0xFF)
        {
            irq.counter = irq.latch;
            nes6502_irq();
        }
        else
        {
            irq.counter++;
        }
    }
}

static void map_write(uint32 address, uint8 value)
{
    if (address == 0x8000)
    {
        command = value & 0x0F;
        return;
    }

    if (address == 0xA000)
    {
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
            mmc_bankvrom(1, 0x1800, value);
            break;

        case 7:
            mmc_bankvrom(1, 0x1C00, value);
            break;

        case 8:
            mmc_bankrom(8, 0x8000, value);
            break;

        case 9:
            mmc_bankrom(8, 0xA000, value);
            break;

        case 10:
            mmc_bankrom(8, 0xC000, value);
            break;

        case 11:
            mmc_bankrom(8, 0xE000, value);
            break;

        case 12:
            switch (value & 3)
            {
                case 0: ppu_setmirroring(PPU_MIRROR_VERT); break;
                case 1: ppu_setmirroring(PPU_MIRROR_HORI); break;
                case 2: ppu_setmirroring(PPU_MIRROR_SCR0); break;
                case 3: ppu_setmirroring(PPU_MIRROR_SCR1); break;
            }
            break;

        case 13:
            break;

        case 14:
            irq.latch = value;
            break;

        case 15:
            irq.enabled = (value & 0x01);
            irq.counter = 0;
            break;

        default:
            break;
        }
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = irq.counter;
    state[1] = irq.latch;
    state[2] = irq.enabled;
    state[3] = command;
}

static void map_setstate(uint8 *state)
{
    irq.counter = state[0];
    irq.latch = state[1];
    irq.enabled = state[2];
    command = state[3];
}

static void map_init(rom_t *cart)
{
    irq.enabled = 0;
    irq.counter = 0;
    irq.latch = 0;
    command = 0;

    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 0);
    mmc_bankrom(8, 0xC000, 0);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
}


mapintf_t map69_intf =
{
    .number     = 69,
    .name       = "Sunsoft FME-7",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = map_hblank,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x8000, 0x8000, map_write },
        { 0xA000, 0xA000, map_write },
    },
};
