#ifndef GBA_CPU_H
#define GBA_CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void (*execute_cycles)(int cycles);
    void (*reset)(void);
    void (*set_mode)(uint32_t mode);
    uint32_t (*get_reg)(int reg_num);
    void (*set_reg)(int reg_num, uint32_t value);
    uint32_t (*get_cpsr)(void);
    void (*set_cpsr)(uint32_t value);
    void (*register_oam_callback)(void (*callback)(bool updated));
    void (*register_interrupt_callback)(void (*callback)(uint32_t irq));
} gba_cpu_interface_t;

extern gba_cpu_interface_t gba_cpu;

void gba_cpu_init(void);

#endif
