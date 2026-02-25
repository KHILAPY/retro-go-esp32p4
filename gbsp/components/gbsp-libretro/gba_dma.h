#ifndef GBA_DMA_H
#define GBA_DMA_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DMA_START_HBLANK,
    DMA_START_VBLANK,
    DMA_START_IMMEDIATE,
    DMA_START_AUDIO
} dma_start_type_t;

typedef struct {
    int (*transfer)(int channel, int *cycles);
    void (*reset)(void);
    void (*enable)(int channel, bool enable);
    void (*set_source)(int channel, uint32_t addr);
    void (*set_dest)(int channel, uint32_t addr);
    void (*set_count)(int channel, uint32_t count);
} gba_dma_interface_t;

extern gba_dma_interface_t gba_dma;

void gba_dma_init(void);

#endif
