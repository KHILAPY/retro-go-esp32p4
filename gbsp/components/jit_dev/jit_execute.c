/*
 * JIT Execution Engine for ESP32-P4
 * 
 * Handles block compilation and execution.
 */

#include "jit_core.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "JIT_EXEC";

extern jit_stats_t g_jit_stats;
extern jit_config_t g_jit_config;

extern void *jit_alloc_code(uint32_t size, bool is_ram);
extern void jit_code_flush(void *addr, size_t size);
extern block_entry_t *jit_block_register(uint32_t pc, bool is_thumb, void *code, uint32_t size, bool l1_only);
extern bool jit_is_ram_address(uint32_t addr);

extern trans_level_t jit_get_translation_level(uint32_t opcode, bool is_thumb);
extern bool jit_translate_block(struct sljit_compiler *C, uint32_t start_pc, bool is_thumb, uint32_t *end_pc);

/* --------------------------------------------------------------------- */
/*  Block Compilation                                                    */
/* --------------------------------------------------------------------- */

static inline bool is_bios_address(uint32_t pc)
{
    return (pc < 0x00004000);
}

block_entry_t *jit_block_compile(uint32_t pc, bool is_thumb)
{
    g_jit_stats.total_compiles++;
    
    if (jit_is_pc_failed(pc, is_thumb)) {
        g_jit_stats.interpreter_fallbacks++;
        return NULL;
    }
    
    if (is_bios_address(pc)) {
        ESP_LOGD(TAG, "Skip BIOS region PC=0x%08X, using interpreter", pc);
        g_jit_stats.interpreter_fallbacks++;
        return NULL;
    }
    
    struct sljit_compiler *C = sljit_create_compiler(NULL);
    if (C == NULL) {
        ESP_LOGE(TAG, "Failed to create SLJIT compiler for PC=0x%08X", pc);
        jit_mark_pc_failed(pc, is_thumb);
        g_jit_stats.compile_failures++;
        return NULL;
    }
    
    sljit_emit_enter(C, 0, SLJIT_ARGS2(W, P, W), 
                     6, 5, 64);
    
    sljit_emit_op1(C, SLJIT_MOV_P, SLJIT_S0, 0, SLJIT_R0, 0);
    
    uint32_t end_pc = pc;
    
    bool l1_only = jit_translate_block(C, pc, is_thumb, &end_pc);
    
    if (end_pc == pc) {
        ESP_LOGI(TAG, "Translation skipped for PC=0x%08X, using interpreter", pc);
        sljit_free_compiler(C);
        jit_mark_pc_failed(pc, is_thumb);
        g_jit_stats.compile_failures++;
        return NULL;
    }
    
    if (!l1_only) {
        ESP_LOGI(TAG, "Non-L1 block at PC=0x%08X, using interpreter", pc);
        sljit_free_compiler(C);
        jit_mark_pc_failed(pc, is_thumb);
        g_jit_stats.compile_failures++;
        return NULL;
    }
    
    void *code = sljit_generate_code(C, 0, NULL);
    size_t code_size = sljit_get_generated_code_size(C);
    
    sljit_free_compiler(C);
    
    if (code == NULL) {
        ESP_LOGE(TAG, "Failed to generate native code");
        jit_mark_pc_failed(pc, is_thumb);
        g_jit_stats.compile_failures++;
        return NULL;
    }
    
    jit_code_flush(code, code_size);
    
    block_entry_t *entry = jit_block_register(pc, is_thumb, code, code_size, l1_only);
    
    if (entry == NULL) {
        ESP_LOGE(TAG, "Failed to register block for PC=0x%08X (cache full?)", pc);
        heap_caps_free(code);
        jit_mark_pc_failed(pc, is_thumb);
        g_jit_stats.cache_full_count++;
        g_jit_stats.compile_failures++;
        return NULL;
    }
    
    // 临时注释 - 调试日志
    // ESP_LOGD(TAG, "Block compiled: PC=0x%08X, size=%u bytes", pc, code_size);
    
    return entry;
}

/* --------------------------------------------------------------------- */
/*  Block Execution                                                      */
/* --------------------------------------------------------------------- */

uint32_t jit_execute_block(block_entry_t *block, gba_cpu_state_t *cpu)
{
    if (block == NULL || block->native_code == NULL || !block->valid) {
        ESP_LOGW(TAG, "Execute block failed: invalid block");
        return cpu->reg[15];
    }
    
    uint32_t old_pc = cpu->reg[15];
    
    typedef uint32_t (*block_func_t)(void *, uint32_t);
    block_func_t func = (block_func_t)block->native_code;
    
    uint32_t new_pc = func(cpu, cpu->cycles_target);
    
    cpu->reg[15] = new_pc;
    
    g_jit_stats.jit_hits++;
    
    if (new_pc != old_pc + 4 && new_pc != old_pc + 8) {
        if (new_pc < 0x1000) {
            ESP_LOGW(TAG, "PC跳转异常: 0x%08X -> 0x%08X (block_pc=0x%08X)", 
                     old_pc, new_pc, block->pc);
        }
    }
    
    return new_pc;
}

/* --------------------------------------------------------------------- */
/*  Translation Level Check                                              */
/* --------------------------------------------------------------------- */

trans_level_t jit_can_translate(uint32_t opcode, bool is_thumb)
{
    return jit_get_translation_level(opcode, is_thumb);
}
