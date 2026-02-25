/*
 * JIT Cache Management for ESP32-P4
 * 
 * Manages translation caches and block hash table.
 */

#include "jit_core.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "hal/cache_ll.h"
#include <string.h>

static const char *TAG = "JIT_CACHE";

/* Global cache pointers */
uint8_t *g_rom_cache = NULL;
uint8_t *g_rom_cache_ptr = NULL;
uint8_t *g_ram_cache = NULL;
uint8_t *g_ram_cache_ptr = NULL;

/* Block hash table */
block_entry_t **g_block_hash = NULL;

/* Statistics */
jit_stats_t g_jit_stats = {0};

/* Configuration */
jit_config_t g_jit_config = {
    .rom_cache_size = GBA_JIT_ROM_CACHE_SIZE,
    .ram_cache_size = GBA_JIT_RAM_CACHE_SIZE,
    .block_hash_size = GBA_JIT_BLOCK_HASH_SIZE,
    .max_block_instructions = GBA_JIT_MAX_BLOCK_INSTRUCTIONS,
    .enable_l1 = 0,
    .enable_l2 = 0,
    .enable_thumb = 0,
    .enable_stats = GBA_JIT_ENABLE_STATS
};

/* Block entry pool for faster allocation */
static block_entry_t *s_block_pool = NULL;
static uint32_t s_block_pool_index = 0;
static uint32_t s_block_pool_size = 0;

/* Failed translation bitmap - tracks PCs that cannot be translated */
#define FAILED_PC_BITMAP_SIZE 8192
static uint32_t *s_failed_pc_bitmap = NULL;

/* --------------------------------------------------------------------- */
/*  Hash Functions                                                       */
/* --------------------------------------------------------------------- */

static inline uint32_t block_hash(uint32_t pc, bool is_thumb)
{
    uint32_t hash = pc >> 2;
    hash ^= (pc >> 10);
    hash ^= (pc >> 18);
    if (is_thumb) {
        hash ^= 0x80000000;
    }
    return hash & (g_jit_config.block_hash_size - 1);
}

static inline void mark_pc_failed(uint32_t pc, bool is_thumb)
{
    if (s_failed_pc_bitmap == NULL) return;
    uint32_t idx = (pc >> 2) & (FAILED_PC_BITMAP_SIZE * 32 - 1);
    if (is_thumb) idx ^= 1;
    s_failed_pc_bitmap[idx / 32] |= (1U << (idx % 32));
}

static inline bool is_pc_failed(uint32_t pc, bool is_thumb)
{
    if (s_failed_pc_bitmap == NULL) return false;
    uint32_t idx = (pc >> 2) & (FAILED_PC_BITMAP_SIZE * 32 - 1);
    if (is_thumb) idx ^= 1;
    return (s_failed_pc_bitmap[idx / 32] & (1U << (idx % 32))) != 0;
}

/* --------------------------------------------------------------------- */
/*  Cache Management                                                     */
/* --------------------------------------------------------------------- */

static void *cache_alloc(uint32_t size, bool is_ram)
{
    uint8_t **ptr = is_ram ? &g_ram_cache_ptr : &g_rom_cache_ptr;
    uint8_t *base = is_ram ? g_ram_cache : g_rom_cache;
    uint32_t max_size = is_ram ? g_jit_config.ram_cache_size : g_jit_config.rom_cache_size;
    
    size = (size + 15) & ~15;
    
    if (*ptr - base + size > max_size) {
        GBA_JIT_LOGE(TAG, "Cache full! Requested %u bytes", size);
        g_jit_stats.cache_full_count++;
        return NULL;
    }
    
    void *code = *ptr;
    *ptr += size;
    
    return code;
}

static void jit_cache_flush(void *addr, size_t size)
{
    if (addr == NULL || size == 0) {
        return;
    }
    
    cache_ll_l1_writeback_dcache_addr(CACHE_LL_ID_ALL, (uint32_t)addr, size);
    cache_ll_l1_invalidate_icache_addr(CACHE_LL_ID_ALL, (uint32_t)addr, size);
    
    cache_ll_writeback_all(1, CACHE_TYPE_DATA, CACHE_LL_ID_ALL);
    cache_ll_invalidate_all(1, CACHE_TYPE_INSTRUCTION, CACHE_LL_ID_ALL);
}

/* --------------------------------------------------------------------- */
/*  Block Pool Management                                                */
/* --------------------------------------------------------------------- */

static int block_pool_init(void)
{
    s_block_pool_size = g_jit_config.block_hash_size * 2;
    s_block_pool = (block_entry_t *)heap_caps_malloc(
        s_block_pool_size * sizeof(block_entry_t),
        MALLOC_CAP_8BIT
    );
    
    if (s_block_pool == NULL) {
        ESP_LOGE(TAG, "Failed to allocate block pool");
        return -1;
    }
    
    memset(s_block_pool, 0, s_block_pool_size * sizeof(block_entry_t));
    s_block_pool_index = 0;
    
    return 0;
}

static void block_pool_deinit(void)
{
    if (s_block_pool != NULL) {
        heap_caps_free(s_block_pool);
        s_block_pool = NULL;
    }
    s_block_pool_index = 0;
    s_block_pool_size = 0;
}

static block_entry_t *block_pool_alloc(void)
{
    if (s_block_pool_index >= s_block_pool_size) {
        return NULL;
    }
    
    block_entry_t *entry = &s_block_pool[s_block_pool_index++];
    memset(entry, 0, sizeof(block_entry_t));
    return entry;
}

/* --------------------------------------------------------------------- */
/*  Public API                                                           */
/* --------------------------------------------------------------------- */

int jit_init(const jit_config_t *config)
{
    if (config != NULL) {
        memcpy(&g_jit_config, config, sizeof(jit_config_t));
    }
    
    g_rom_cache = heap_caps_malloc(
        g_jit_config.rom_cache_size,
        MALLOC_CAP_SPIRAM
    );
    
    if (g_rom_cache == NULL) {
        ESP_LOGE(TAG, "Failed to allocate ROM translation cache (%u KB)",
                 g_jit_config.rom_cache_size / 1024);
        return -1;
    }
    
    g_ram_cache = heap_caps_malloc(
        g_jit_config.ram_cache_size,
        MALLOC_CAP_SPIRAM
    );
    
    if (g_ram_cache == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RAM translation cache (%u KB)",
                 g_jit_config.ram_cache_size / 1024);
        heap_caps_free(g_rom_cache);
        g_rom_cache = NULL;
        return -1;
    }
    
    g_block_hash = (block_entry_t **)heap_caps_malloc(
        g_jit_config.block_hash_size * sizeof(block_entry_t *),
        MALLOC_CAP_8BIT
    );
    
    if (g_block_hash == NULL) {
        ESP_LOGE(TAG, "Failed to allocate block hash table");
        heap_caps_free(g_rom_cache);
        heap_caps_free(g_ram_cache);
        g_rom_cache = NULL;
        g_ram_cache = NULL;
        return -1;
    }
    
    if (block_pool_init() != 0) {
        heap_caps_free(g_rom_cache);
        heap_caps_free(g_ram_cache);
        heap_caps_free(g_block_hash);
        g_rom_cache = NULL;
        g_ram_cache = NULL;
        g_block_hash = NULL;
        return -1;
    }
    
    s_failed_pc_bitmap = (uint32_t *)heap_caps_malloc(
        FAILED_PC_BITMAP_SIZE * sizeof(uint32_t),
        MALLOC_CAP_8BIT
    );
    if (s_failed_pc_bitmap == NULL) {
        ESP_LOGW(TAG, "Failed to allocate failed PC bitmap, will retry failed translations");
    } else {
        memset(s_failed_pc_bitmap, 0, FAILED_PC_BITMAP_SIZE * sizeof(uint32_t));
    }
    
    g_rom_cache_ptr = g_rom_cache;
    g_ram_cache_ptr = g_ram_cache;
    
    memset(g_block_hash, 0, g_jit_config.block_hash_size * sizeof(block_entry_t *));
    memset(&g_jit_stats, 0, sizeof(jit_stats_t));
    
    ESP_LOGI(TAG, "JIT cache initialized: ROM=%uKB, RAM=%uKB, Hash=%u, Pool=%u",
             g_jit_config.rom_cache_size / 1024, g_jit_config.ram_cache_size / 1024,
             g_jit_config.block_hash_size, s_block_pool_size);
    
    return 0;
}

void jit_deinit(void)
{
    if (g_rom_cache != NULL) {
        heap_caps_free(g_rom_cache);
        g_rom_cache = NULL;
        g_rom_cache_ptr = NULL;
    }
    
    if (g_ram_cache != NULL) {
        heap_caps_free(g_ram_cache);
        g_ram_cache = NULL;
        g_ram_cache_ptr = NULL;
    }
    
    if (g_block_hash != NULL) {
        heap_caps_free(g_block_hash);
        g_block_hash = NULL;
    }
    
    block_pool_deinit();
}

void jit_reset(void)
{
    g_rom_cache_ptr = g_rom_cache;
    g_ram_cache_ptr = g_ram_cache;
    
    memset(g_block_hash, 0, g_jit_config.block_hash_size * sizeof(block_entry_t *));
    memset(&g_jit_stats, 0, sizeof(jit_stats_t));
    s_block_pool_index = 0;
}

void jit_flush_cache(void)
{
    jit_reset();
}

block_entry_t *jit_block_lookup(uint32_t pc, bool is_thumb)
{
    if (is_pc_failed(pc, is_thumb)) {
        g_jit_stats.interpreter_fallbacks++;
        return NULL;
    }
    
    if (pc < 0x00004000) {
        g_jit_stats.interpreter_fallbacks++;
        return NULL;
    }
    
    uint32_t idx = block_hash(pc, is_thumb);
    block_entry_t *entry = g_block_hash[idx];
    
    while (entry != NULL) {
        if (entry->pc == pc && entry->is_thumb == is_thumb && entry->valid) {
            entry->exec_count++;
            g_jit_stats.jit_hits++;
            g_jit_stats.cache_hits++;
            return entry;
        }
        entry = entry->next;
    }
    
    g_jit_stats.interpreter_fallbacks++;
    g_jit_stats.cache_misses++;
    return NULL;
}

block_entry_t *jit_block_register(uint32_t pc, bool is_thumb, void *code, uint32_t size, bool l1_only)
{
    uint32_t idx = block_hash(pc, is_thumb);
    
    block_entry_t *entry = block_pool_alloc();
    if (entry == NULL) {
        ESP_LOGE(TAG, "Block pool exhausted");
        return NULL;
    }
    
    entry->pc = pc;
    entry->native_code = (uint8_t *)code;
    entry->code_size = size;
    entry->is_thumb = is_thumb;
    entry->valid = 1;
    entry->l1_only = l1_only;
    entry->exec_count = 0;
    
    entry->next = g_block_hash[idx];
    g_block_hash[idx] = entry;
    
    g_jit_stats.total_blocks++;
    g_jit_stats.compile_successes++;
    
    return entry;
}

void jit_block_invalidate(uint32_t pc, bool is_thumb)
{
    uint32_t idx = block_hash(pc, is_thumb);
    block_entry_t *entry = g_block_hash[idx];
    block_entry_t *prev = NULL;
    
    while (entry != NULL) {
        if (entry->pc == pc && entry->is_thumb == is_thumb) {
            entry->valid = 0;
            if (prev != NULL) {
                prev->next = entry->next;
            } else {
                g_block_hash[idx] = entry->next;
            }
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void jit_mark_pc_failed(uint32_t pc, bool is_thumb)
{
    bool was_failed = is_pc_failed(pc, is_thumb);
    mark_pc_failed(pc, is_thumb);
    if (!was_failed) {
        g_jit_stats.failed_pc_count++;
        ESP_LOGD(TAG, "Marked PC=0x%08X as failed (total failed PCs: %u)", 
                 pc, g_jit_stats.failed_pc_count);
    }
}

bool jit_is_pc_failed(uint32_t pc, bool is_thumb)
{
    return is_pc_failed(pc, is_thumb);
}

void *jit_alloc_code(uint32_t size, bool is_ram)
{
    return cache_alloc(size, is_ram);
}

void jit_code_flush(void *addr, size_t size)
{
    jit_cache_flush(addr, size);
}

void jit_get_stats(jit_stats_t *stats)
{
    if (stats != NULL) {
        memcpy(stats, &g_jit_stats, sizeof(jit_stats_t));
    }
}

void jit_print_stats(void)
{
    ESP_LOGI(TAG, "=== JIT Statistics ===");
    ESP_LOGI(TAG, "Total blocks: %u", g_jit_stats.total_blocks);
    ESP_LOGI(TAG, "Total compiles: %u", g_jit_stats.total_compiles);
    ESP_LOGI(TAG, "JIT hits: %u", g_jit_stats.jit_hits);
    ESP_LOGI(TAG, "Interpreter fallbacks: %u", g_jit_stats.interpreter_fallbacks);
    ESP_LOGI(TAG, "Cache full count: %u", g_jit_stats.cache_full_count);
    ESP_LOGI(TAG, "L1 instructions: %u", g_jit_stats.l1_instructions);
    ESP_LOGI(TAG, "L2 instructions: %u", g_jit_stats.l2_instructions);
    ESP_LOGI(TAG, "Complex instructions: %u", g_jit_stats.complex_instructions);
    
    if (g_jit_stats.jit_hits + g_jit_stats.interpreter_fallbacks > 0) {
        uint32_t hit_rate = g_jit_stats.jit_hits * 100 / 
                           (g_jit_stats.jit_hits + g_jit_stats.interpreter_fallbacks);
        ESP_LOGI(TAG, "JIT hit rate: %u%%", hit_rate);
    }
    
    if (g_rom_cache != NULL && g_rom_cache_ptr != NULL) {
        uint32_t used = g_rom_cache_ptr - g_rom_cache;
        ESP_LOGI(TAG, "ROM cache used: %u / %u bytes (%u%%)",
                 used, g_jit_config.rom_cache_size,
                 used * 100 / g_jit_config.rom_cache_size);
    }
    
    if (g_ram_cache != NULL && g_ram_cache_ptr != NULL) {
        uint32_t used = g_ram_cache_ptr - g_ram_cache;
        ESP_LOGI(TAG, "RAM cache used: %u / %u bytes (%u%%)",
                 used, g_jit_config.ram_cache_size,
                 used * 100 / g_jit_config.ram_cache_size);
    }
}

bool jit_is_ram_address(uint32_t addr)
{
    if (addr >= 0x02000000 && addr < 0x02040000) return true;
    if (addr >= 0x03000000 && addr < 0x03008000) return true;
    return false;
}
