#ifndef GBA_PPU_H
#define GBA_PPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void (*render_scanline)(uint32_t vcount);
    uint16_t* (*get_screen_pixels)(void);
    void (*reset)(void);
    void (*set_mode)(uint32_t mode);
    void (*set_palette)(uint16_t *palette);
    void (*force_refresh)(void);
} gba_ppu_interface_t;

extern gba_ppu_interface_t gba_ppu;

void gba_ppu_init(void);

#endif
