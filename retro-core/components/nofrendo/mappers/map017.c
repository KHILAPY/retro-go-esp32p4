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
** map017.c: FFE F8xxx mapper interface
**
** FFE Copier Mapper - Extended mode with 8K PRG banks and 1K CHR banks
** Used by: Dragonball Z 2/3 [hacked], Dynamite Batman 2 [Hacked],
**          Parodius [Hacked], Saiyuuki World 2 [Hacked]
*/

#include "nes/nes.h"

static uint8 preg[4];
static uint8 creg[8];
static uint8 mirr;
static bool irq_enabled;
static int32 irq_counter;

static void sync(void)
{
    for (int i = 0; i < 8; i++)
        mmc_bankchr(1, i << 10, creg[i], CHR_ANY);
    
    mmc_bankrom(8, 0x8000, preg[0]);
    mmc_bankrom(8, 0xA000, preg[1]);
    mmc_bankrom(8, 0xC000, preg[2]);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
    
    switch (mirr)
    {
    case 0: ppu_setmirroring(PPU_MIRROR_SCR0); break;
    case 1: ppu_setmirroring(PPU_MIRROR_SCR1); break;
    case 2: ppu_setmirroring(PPU_MIRROR_VERT); break;
    case 3: ppu_setmirroring(PPU_MIRROR_HORI); break;
    }
}

static void map_write_mirr(uint32 address, uint8 value)
{
    mirr = ((address << 1) & 2) | ((value >> 4) & 1);
    sync();
}

static void map_write_irq(uint32 address, uint8 value)
{
    switch (address)
    {
    case 0x4501:
        irq_enabled = false;
        break;
    case 0x4502:
        irq_counter = (irq_counter & 0xFF00) | value;
        break;
    case 0x4503:
        irq_counter = (irq_counter & 0x00FF) | (value << 8);
        irq_enabled = true;
        break;
    }
}

static void map_write_prg(uint32 address, uint8 value)
{
    preg[address & 3] = value;
    sync();
}

static void map_write_chr(uint32 address, uint8 value)
{
    creg[address & 7] = value;
    sync();
}

static void map_hblank(nes_t *nes)
{
    if (nes->scanline > 240)
        return;

    if (!ppu_enabled())
        return;

    if (!irq_enabled)
        return;
    
    irq_counter += 114;
    if (irq_counter >= 0x10000)
    {
        irq_enabled = false;
        irq_counter = 0;
        nes6502_irq();
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = preg[0];
    state[1] = preg[1];
    state[2] = preg[2];
    state[3] = preg[3];
    state[4] = mirr;
    state[5] = irq_enabled;
    state[6] = irq_counter & 0xFF;
    state[7] = (irq_counter >> 8) & 0xFF;
    state[8] = (irq_counter >> 16) & 0xFF;
    for (int i = 0; i < 8; i++)
        state[9 + i] = creg[i];
}

static void map_setstate(uint8 *state)
{
    preg[0] = state[0];
    preg[1] = state[1];
    preg[2] = state[2];
    preg[3] = state[3];
    mirr = state[4];
    irq_enabled = state[5];
    irq_counter = state[6] | (state[7] << 8) | (state[8] << 16);
    for (int i = 0; i < 8; i++)
        creg[i] = state[9 + i];
    sync();
}

static void map_init(rom_t *cart)
{
    preg[0] = 0;
    preg[1] = 1;
    preg[2] = 2;
    preg[3] = 3;
    
    for (int i = 0; i < 8; i++)
        creg[i] = i;
    
    mirr = 2;
    irq_enabled = false;
    irq_counter = 0;
    
    mmc_bankprg(8, 0x6000, 0, PRG_RAM);
    sync();
}

mapintf_t map17_intf =
{
    .number     = 17,
    .name       = "FFE F8xxx",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = map_hblank,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x42FE, 0x42FF, map_write_mirr },
        { 0x4500, 0x4503, map_write_irq },
        { 0x4504, 0x4507, map_write_prg },
        { 0x4510, 0x4517, map_write_chr }
    },
};
