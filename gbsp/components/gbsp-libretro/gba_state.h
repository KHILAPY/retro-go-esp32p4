#ifndef GBA_STATE_H
#define GBA_STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t regs[64];
    uint32_t mode;
    uint32_t cpsr;
    uint32_t spsr[6];
    uint32_t halt_state;
} gba_cpu_state_t;

typedef struct {
    uint32_t vcount;
    uint32_t dispstat;
    uint16_t *screen_buffer;
    bool scanline_done;
    bool frame_done;
} gba_ppu_state_t;

typedef struct {
    uint32_t source_addr;
    uint32_t dest_addr;
    uint32_t count;
    uint32_t control;
    bool enabled;
} gba_dma_channel_t;

typedef struct {
    gba_dma_channel_t channels[4];
} gba_dma_state_t;

typedef struct {
    uint32_t frame_counter;
    uint32_t cpu_ticks;
    uint32_t execute_cycles;
    uint32_t video_count;
    uint32_t irq_raised;
} gba_system_state_t;

typedef struct {
    gba_cpu_state_t cpu;
    gba_ppu_state_t ppu;
    gba_dma_state_t dma;
    gba_system_state_t system;
} gba_state_t;

gba_state_t* gba_get_state(void);
void gba_lock_state(void);
void gba_unlock_state(void);
void gba_state_init(void);

#endif
