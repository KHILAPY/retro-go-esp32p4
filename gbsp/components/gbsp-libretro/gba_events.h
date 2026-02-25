#ifndef GBA_EVENTS_H
#define GBA_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENT_SCANLINE_START,
    EVENT_SCANLINE_END,
    EVENT_HBLANK_START,
    EVENT_VBLANK_START,
    EVENT_DMA_TRANSFER,
    EVENT_INTERRUPT,
    EVENT_OAM_UPDATE
} gba_event_type_t;

typedef void (*gba_event_handler_t)(gba_event_type_t type, void *data);

void gba_register_event_handler(gba_event_type_t type, gba_event_handler_t handler);
void gba_trigger_event(gba_event_type_t type, void *data);
void gba_events_init(void);

#endif
