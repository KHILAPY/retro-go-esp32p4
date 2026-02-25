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
** map245.c: WAIXING MMC3 +EX.PRG (Mapper 245)
**
** Hardware: Modified MMC3 with:
**   - PPU A10,A11,A12 disconnected from MMC3 (IRQ disabled)
**   - Fixed vertical mirroring
**   - CHR-RAM instead of CHR-ROM
**   - Outer bank register at indirect register 0
**
** Reference: https://nesdev.nes.science/f28/t13183.xhtml
**
*/

#include "nes/nes.h"

static uint8 reg8000;
static uint8 outer_bank;

static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xE001)
    {
    case 0x8000:
        reg8000 = value;
        mmc_bankrom(8, (value & 0x40) ? 0x8000 : 0xC000, -2);
        break;

    case 0x8001:
        switch (reg8000 & 0x07)
        {
        case 0:
            outer_bank = value;
            break;

        case 1:
            break;

        case 2:
        case 3:
        case 4:
        case 5:
            break;

        case 6:
            mmc_bankrom(8, (reg8000 & 0x40) ? 0xC000 : 0x8000, (outer_bank << 4) | value);
            break;

        case 7:
            mmc_bankrom(8, 0xA000, (outer_bank << 4) | value);
            break;
        }
        break;

    case 0xA000:
        break;

    case 0xA001:
        break;

    case 0xC000:
    case 0xC001:
    case 0xE000:
    case 0xE001:
        break;

    default:
        break;
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = reg8000;
    state[1] = outer_bank;
}

static void map_setstate(uint8 *state)
{
    reg8000 = state[0];
    outer_bank = state[1];
}

static void map_init(rom_t *cart)
{
    reg8000 = 0;
    outer_bank = 0;

    if (cart->chr_rom_banks > 0)
    {
        MESSAGE_INFO("Mapper 245: Converting CHR-ROM to CHR-RAM (%d banks)\n", cart->chr_rom_banks);
        cart->chr_ram = realloc(cart->chr_ram, 0x2000 * cart->chr_rom_banks);
        cart->chr_ram_banks = cart->chr_rom_banks;
        memcpy(cart->chr_ram, cart->chr_rom, 0x2000 * cart->chr_rom_banks);
        cart->chr_rom_banks = 0;
    }

    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 1);
    mmc_bankrom(8, 0xC000, -2);
    mmc_bankrom(8, 0xE000, -1);

    mmc_bankchr(8, 0x0000, 0, CHR_RAM);

    ppu_setmirroring(PPU_MIRROR_VERT);
}


mapintf_t map245_intf =
{
    .number     = 245,
    .name       = "WAIXING MMC3",
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
