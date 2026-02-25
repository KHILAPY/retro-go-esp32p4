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
** map090.c: Mapper 90 (JY Company) interface
** Used by: Various pirate games and multi-carts
** 
** JY Company mapper used in many pirate cartridges and multi-carts.
** Supports PRG/CHR banking, IRQ, and various mirroring modes.
**
** Register map:
** $5000-$5FFF: Additional control registers (extended features)
** $8000-$8007: CHR bank registers (1KB each)
** $9000-$9003: PRG bank registers (8KB each)
** $9004: Mirroring control
** $9005: IRQ latch value
** $9006: IRQ counter reload
** $9007: IRQ enable/disable
**
*/

#include "nes/nes.h"

static struct
{
    uint8 prg_bank[4];
    uint8 chr_bank[8];
    uint8 mirroring;
    uint8 irq_counter;
    uint8 irq_latch;
    bool irq_enabled;
    bool irq_reload_pending;
    uint8 ext_mode;
} state;


static void update_prg(void)
{
    mmc_bankrom(8, 0x8000, state.prg_bank[0]);
    mmc_bankrom(8, 0xA000, state.prg_bank[1]);
    mmc_bankrom(8, 0xC000, state.prg_bank[2]);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
}

static void update_chr(void)
{
    for (int i = 0; i < 8; i++)
    {
        mmc_bankvrom(1, i * 0x400, state.chr_bank[i]);
    }
}

static void update_mirroring(void)
{
    switch (state.mirroring & 0x03)
    {
        case 0: ppu_setmirroring(PPU_MIRROR_VERT); break;
        case 1: ppu_setmirroring(PPU_MIRROR_HORI); break;
        case 2: ppu_setmirroring(PPU_MIRROR_SCR0); break;
        case 3: ppu_setmirroring(PPU_MIRROR_SCR1); break;
    }
}

static void map_write(uint32 address, uint8 value)
{
    if (address >= 0x5000 && address < 0x6000)
    {
        state.ext_mode = value;
        return;
    }

    switch (address & 0xF007)
    {
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003:
    case 0x8004:
    case 0x8005:
    case 0x8006:
    case 0x8007:
        state.chr_bank[address & 0x07] = value;
        update_chr();
        break;

    case 0x9000:
    case 0x9001:
    case 0x9002:
    case 0x9003:
        state.prg_bank[address & 0x03] = value;
        update_prg();
        break;

    case 0x9004:
        state.mirroring = value;
        update_mirroring();
        break;

    case 0x9005:
        state.irq_latch = value;
        break;

    case 0x9006:
        state.irq_counter = state.irq_latch;
        state.irq_reload_pending = false;
        break;

    case 0x9007:
        state.irq_enabled = (value & 0x01) != 0;
        if (state.irq_enabled)
        {
            state.irq_reload_pending = true;
        }
        break;

    default:
        break;
    }
}

static void map_hblank(nes_t *nes)
{
    if (nes->scanline > 240)
        return;

    if (!ppu_enabled())
        return;

    if (state.irq_enabled)
    {
        if (state.irq_reload_pending)
        {
            state.irq_counter = state.irq_latch;
            state.irq_reload_pending = false;
        }
        
        if (state.irq_counter > 0)
        {
            state.irq_counter--;
            if (state.irq_counter == 0)
            {
                nes6502_irq();
                state.irq_counter = state.irq_latch;
            }
        }
    }
}

static void map_getstate(uint8 *state_buf)
{
    state_buf[0] = state.prg_bank[0];
    state_buf[1] = state.prg_bank[1];
    state_buf[2] = state.prg_bank[2];
    state_buf[3] = state.prg_bank[3];
    state_buf[4] = state.mirroring;
    state_buf[5] = state.irq_counter;
    state_buf[6] = state.irq_latch;
    state_buf[7] = state.irq_enabled ? 1 : 0;
    state_buf[8] = state.ext_mode;
    for (int i = 0; i < 8; i++)
        state_buf[9 + i] = state.chr_bank[i];
}

static void map_setstate(uint8 *state_buf)
{
    state.prg_bank[0] = state_buf[0];
    state.prg_bank[1] = state_buf[1];
    state.prg_bank[2] = state_buf[2];
    state.prg_bank[3] = state_buf[3];
    state.mirroring = state_buf[4];
    state.irq_counter = state_buf[5];
    state.irq_latch = state_buf[6];
    state.irq_enabled = state_buf[7] != 0;
    state.ext_mode = state_buf[8];
    for (int i = 0; i < 8; i++)
        state.chr_bank[i] = state_buf[9 + i];
    update_prg();
    update_chr();
    update_mirroring();
}

static void map_init(rom_t *cart)
{
    memset(&state, 0, sizeof(state));

    state.prg_bank[0] = 0;
    state.prg_bank[1] = 1;
    state.prg_bank[2] = (MMC_LASTBANK - 1) & 0x7F;
    state.prg_bank[3] = MMC_LASTBANK & 0x7F;

    for (int i = 0; i < 8; i++)
        state.chr_bank[i] = i;

    state.mirroring = 0;
    state.irq_counter = 0;
    state.irq_latch = 0;
    state.irq_enabled = false;
    state.irq_reload_pending = false;
    state.ext_mode = 0;

    update_prg();
    update_chr();
    update_mirroring();
}


mapintf_t map90_intf =
{
    .number     = 90,
    .name       = "JY Company",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = map_hblank,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x5000, 0x5FFF, map_write },
        { 0x8000, 0xFFFF, map_write }
    },
};
