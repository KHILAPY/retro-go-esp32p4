#pragma once

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#include <esp_attr.h>
#include <esp_memory_utils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_async_memcpy.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RG_PERF_CORE_0      0
#define RG_PERF_CORE_1      1
#define RG_PERF_NO_AFFINITY -1

#ifdef ESP_PLATFORM

#define RG_PERF_CURRENT_CORE()      xPortGetCoreID()
#define RG_PERF_OTHER_CORE()        (1 - xPortGetCoreID())

static inline void *rg_perf_alloc_dma(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
}

static inline void *rg_perf_alloc_sram(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static inline void *rg_perf_alloc_psram(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static inline void *rg_perf_alloc_prefetch(size_t size)
{
    return heap_caps_malloc_prefer(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, MALLOC_CAP_SPIRAM);
}

static inline bool rg_perf_is_psram(const void *ptr)
{
    return ptr && esp_ptr_external_ram(ptr);
}

static inline bool rg_perf_is_sram(const void *ptr)
{
    return ptr && !esp_ptr_external_ram(ptr);
}

static inline size_t rg_perf_get_free_sram(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static inline size_t rg_perf_get_free_psram(void)
{
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

static inline size_t rg_perf_get_largest_free_sram(void)
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

static inline size_t rg_perf_get_largest_free_psram(void)
{
    return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
}

typedef struct {
    async_memcpy_handle_t handle;
    volatile bool transfer_done;
} rg_dma_memcpy_t;

static inline bool rg_dma_memcpy_init(rg_dma_memcpy_t *ctx)
{
    async_memcpy_config_t config = {
        .backlog = 8,
        .sram_trans_align = 4,
        .psram_trans_align = 32,
        .flags = 0,
    };
    return esp_async_memcpy_install(&config, &ctx->handle) == ESP_OK;
}

static inline void rg_dma_memcpy_deinit(rg_dma_memcpy_t *ctx)
{
    if (ctx->handle) {
        esp_async_memcpy_uninstall(ctx->handle);
        ctx->handle = NULL;
    }
}

static inline bool rg_dma_memcpy_async(rg_dma_memcpy_t *ctx, void *dst, const void *src, size_t size)
{
    ctx->transfer_done = false;
    return esp_async_memcpy(ctx->handle, dst, (void *)src, size, NULL, NULL) == ESP_OK;
}

static inline bool rg_dma_memcpy_is_done(rg_dma_memcpy_t *ctx)
{
    return ctx->transfer_done;
}

#else

#define RG_PERF_CURRENT_CORE()      0
#define RG_PERF_OTHER_CORE()        0
#define rg_perf_alloc_dma(size)     malloc(size)
#define rg_perf_alloc_sram(size)    malloc(size)
#define rg_perf_alloc_psram(size)   malloc(size)
#define rg_perf_alloc_prefetch(size) malloc(size)
#define rg_perf_is_psram(ptr)       false
#define rg_perf_is_sram(ptr)        true
#define rg_perf_get_free_sram()     (1024 * 1024)
#define rg_perf_get_free_psram()    0
#define rg_perf_get_largest_free_sram() (1024 * 1024)
#define rg_perf_get_largest_free_psram() 0

typedef struct {
    void *handle;
    volatile bool transfer_done;
} rg_dma_memcpy_t;

static inline bool rg_dma_memcpy_init(rg_dma_memcpy_t *ctx) { ctx->handle = NULL; return true; }
static inline void rg_dma_memcpy_deinit(rg_dma_memcpy_t *ctx) { }
static inline bool rg_dma_memcpy_async(rg_dma_memcpy_t *ctx, void *dst, const void *src, size_t size) { memcpy(dst, src, size); return true; }
static inline bool rg_dma_memcpy_is_done(rg_dma_memcpy_t *ctx) { return true; }

#endif

#ifdef __cplusplus
}
#endif
