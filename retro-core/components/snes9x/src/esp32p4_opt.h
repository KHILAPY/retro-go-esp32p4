#ifndef _ESP32P4_OPT_H_
#define _ESP32P4_OPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef CONFIG_IDF_TARGET_ESP32P4

#define ESP32P4_OPTIMIZED 1

#define SNES_VEC_ENABLE_UNUSED_FUNCS 0

#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define prefetch_read(addr)  __builtin_prefetch(addr, 0, 0)
#define prefetch_write(addr) __builtin_prefetch(addr, 1, 0)

#define prefetch_opcode(pc)    __builtin_prefetch(pc, 0, 3)
#define prefetch_handler(ptr)  __builtin_prefetch(ptr, 0, 3)

#define ESP_LOOP_UNROLL_4
#define ESP_LOOP_UNROLL_8

#define ESP_HOT              __attribute__((hot))
#define ESP_COLD             __attribute__((cold))
#define ESP_ALIGNED(n)       __attribute__((aligned(n)))
#define ESP_SECTION(s)       __attribute__((section(s)))

#define ESP_CACHE_LINE_SIZE      64
#define ESP_CACHE_ALIGNED_ATTR   __attribute__((aligned(ESP_CACHE_LINE_SIZE)))
#define ESP_CACHE_ALIGNED_VAR    __attribute__((aligned(ESP_CACHE_LINE_SIZE)))

#define ESP_DMA_ALIGNED          __attribute__((aligned(64)))
#define ESP_DMA_CAPABLE(ptr)     (((uintptr_t)(ptr) & 63) == 0)

#define ESP_ALWAYS_INLINE    __attribute__((always_inline)) inline
#define ESP_FLATTEN          __attribute__((flatten))
#define ESP_PURE             __attribute__((pure))
#define ESP_CONST            __attribute__((const))

#define ESP_BRANCH_HINT_TAKEN    __builtin_expect_with_probability(1, 1, 0.9)
#define ESP_BRANCH_HINT_NOT_TAKEN __builtin_expect_with_probability(0, 0, 0.9)

#define ESP_PREFETCH_NEXT_OPCODE(pc, table, op) \
    do { \
        __builtin_prefetch(pc + 1, 0, 3); \
        __builtin_prefetch(&table[op].S9xOpcode, 0, 3); \
    } while(0)

typedef uint8_t  v8u8   __attribute__((vector_size(8),  may_alias));
typedef uint16_t v8u16  __attribute__((vector_size(16), may_alias));
typedef uint32_t v8u32  __attribute__((vector_size(32), may_alias));
typedef int8_t   v8i8   __attribute__((vector_size(8),  may_alias));
typedef int16_t  v8i16  __attribute__((vector_size(16), may_alias));
typedef int32_t  v8i32  __attribute__((vector_size(32), may_alias));

#define ESP_VEC_LOAD(ptr)        (*(const v8u8*)(ptr))
#define ESP_VEC_STORE(ptr, val)  (*(v8u8*)(ptr) = (val))

#define ESP_VEC_SET1_8(val)      ((v8u8){(val), (val), (val), (val), (val), (val), (val), (val)})
#define ESP_VEC_SET1_16(val)     ((v8u16){(val), (val), (val), (val), (val), (val), (val), (val)})

#define ESP_VEC_ADD(a, b)        ((a) + (b))
#define ESP_VEC_SUB(a, b)        ((a) - (b))
#define ESP_VEC_MUL(a, b)        ((a) * (b))
#define ESP_VEC_AND(a, b)        ((a) & (b))
#define ESP_VEC_OR(a, b)         ((a) | (b))
#define ESP_VEC_XOR(a, b)        ((a) ^ (b))
#define ESP_VEC_NOT(a)           (~(a))
#define ESP_VEC_SHL(a, n)        ((a) << (n))
#define ESP_VEC_SHR(a, n)        ((a) >> (n))

#if SNES_VEC_ENABLE_UNUSED_FUNCS

static ESP_ALWAYS_INLINE void esp_vec_memcpy8(void *dst, const void *src, size_t count)
{
    for (size_t i = 0; i < count; i += 8) {
        v8u8 val = ESP_VEC_LOAD((const uint8_t*)src + i);
        ESP_VEC_STORE((uint8_t*)dst + i, val);
    }
}

static ESP_ALWAYS_INLINE void esp_vec_memset8(void *dst, uint8_t val, size_t count)
{
    v8u8 vec = ESP_VEC_SET1_8(val);
    for (size_t i = 0; i < count; i += 8) {
        ESP_VEC_STORE((uint8_t*)dst + i, vec);
    }
}

static ESP_ALWAYS_INLINE void esp_vec_blend565(uint16_t *dst, const uint16_t *src1,
                                                const uint16_t *src2, uint8_t alpha,
                                                size_t count)
{
    const uint16_t inv_alpha = 256 - alpha;

    for (size_t i = 0; i < count; i++) {
        uint16_t p1 = src1[i];
        uint16_t p2 = src2[i];

        uint16_t r1 = (p1 >> 11) & 0x1F;
        uint16_t g1 = (p1 >> 5) & 0x3F;
        uint16_t b1 = p1 & 0x1F;

        uint16_t r2 = (p2 >> 11) & 0x1F;
        uint16_t g2 = (p2 >> 5) & 0x3F;
        uint16_t b2 = p2 & 0x1F;

        uint16_t r = (r1 * alpha + r2 * inv_alpha) >> 8;
        uint16_t g = (g1 * alpha + g2 * inv_alpha) >> 8;
        uint16_t b = (b1 * alpha + b2 * inv_alpha) >> 8;

        dst[i] = (r << 11) | (g << 5) | b;
    }
}

static ESP_ALWAYS_INLINE void esp_vec_brightness565(uint16_t *dst, const uint16_t *src,
                                                     uint8_t brightness, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uint16_t pixel = src[i];
        uint16_t r = ((pixel >> 11) & 0x1F) * brightness >> 8;
        uint16_t g = ((pixel >> 5) & 0x3F) * brightness >> 8;
        uint16_t b = (pixel & 0x1F) * brightness >> 8;
        dst[i] = (r << 11) | (g << 5) | b;
    }
}

#endif

#endif

static ESP_ALWAYS_INLINE void esp_fast_memcpy(void *dst, const void *src, size_t size)
{
    memcpy(dst, src, size);
}

static ESP_ALWAYS_INLINE void esp_fast_memcpy_line(void *dst, const void *src, size_t line_size,
                                                    int num_lines, int dst_stride, int src_stride)
{
    if (num_lines == 1 || dst_stride == src_stride) {
        memcpy(dst, src, line_size * num_lines);
        return;
    }
    for (int i = 0; i < num_lines; i++) {
        memcpy((uint8_t*)dst + i * dst_stride,
               (const uint8_t*)src + i * src_stride, line_size);
    }
}

static ESP_ALWAYS_INLINE void esp_fast_memset(void *dst, uint8_t val, size_t size)
{
    memset(dst, val, size);
}

#ifdef __cplusplus
}
#endif

#endif
