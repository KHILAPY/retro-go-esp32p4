#ifndef _PIXEL_OPT_H_
#define _PIXEL_OPT_H_

#include "pixform.h"
#include "esp32p4_opt.h"

#ifdef __cplusplus
extern "C" {
#endif

static ESP_ALWAYS_INLINE uint16_t esp_pixel_blend565(uint16_t src, uint16_t dst, uint16_t alpha)
{
    const uint16_t inv_alpha = 32 - alpha;
    
    uint16_t src_r = (src >> 11) & 0x1F;
    uint16_t src_g = (src >> 5) & 0x3F;
    uint16_t src_b = src & 0x1F;
    
    uint16_t dst_r = (dst >> 11) & 0x1F;
    uint16_t dst_g = (dst >> 5) & 0x3F;
    uint16_t dst_b = dst & 0x1F;
    
    uint16_t r = (src_r * alpha + dst_r * inv_alpha) >> 5;
    uint16_t g = (src_g * alpha + dst_g * inv_alpha) >> 6;
    uint16_t b = (src_b * alpha + dst_b * inv_alpha) >> 5;
    
    return (r << 11) | (g << 5) | b;
}

static ESP_ALWAYS_INLINE uint16_t esp_pixel_add565(uint16_t src, uint16_t dst)
{
    uint16_t src_r = (src >> 11) & 0x1F;
    uint16_t src_g = (src >> 5) & 0x3F;
    uint16_t src_b = src & 0x1F;
    
    uint16_t dst_r = (dst >> 11) & 0x1F;
    uint16_t dst_g = (dst >> 5) & 0x3F;
    uint16_t dst_b = dst & 0x1F;
    
    uint16_t r = (src_r + dst_r);
    uint16_t g = (src_g + dst_g);
    uint16_t b = (src_b + dst_b);
    
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    
    return (r << 11) | (g << 5) | b;
}

static ESP_ALWAYS_INLINE uint16_t esp_pixel_sub565(uint16_t src, uint16_t dst)
{
    int16_t src_r = (src >> 11) & 0x1F;
    int16_t src_g = (src >> 5) & 0x3F;
    int16_t src_b = src & 0x1F;
    
    int16_t dst_r = (dst >> 11) & 0x1F;
    int16_t dst_g = (dst >> 5) & 0x3F;
    int16_t dst_b = dst & 0x1F;
    
    int16_t r = dst_r - src_r;
    int16_t g = dst_g - src_g;
    int16_t b = dst_b - src_b;
    
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    
    return (r << 11) | (g << 5) | b;
}

static ESP_ALWAYS_INLINE uint16_t esp_pixel_brightness565(uint16_t pixel, uint16_t brightness)
{
    uint16_t r = ((pixel >> 11) & 0x1F) * brightness >> 5;
    uint16_t g = ((pixel >> 5) & 0x3F) * brightness >> 6;
    uint16_t b = (pixel & 0x1F) * brightness >> 5;
    
    return (r << 11) | (g << 5) | b;
}

static ESP_ALWAYS_INLINE void esp_pixel_fill_line565(uint16_t *dst, uint16_t value, size_t count)
{
    ESP_LOOP_UNROLL_8
    for (size_t i = 0; i < count; i++) {
        dst[i] = value;
    }
}

static ESP_ALWAYS_INLINE void esp_pixel_copy_line565(uint16_t *dst, const uint16_t *src, size_t count)
{
    ESP_LOOP_UNROLL_8
    for (size_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

#define COLOR_ADD_OPT(src, dst)     esp_pixel_add565(src, dst)
#define COLOR_SUB_OPT(src, dst)     esp_pixel_sub565(src, dst)
#define COLOR_BLEND_OPT(src, dst, a) esp_pixel_blend565(src, dst, a)
#define COLOR_BRIGHT_OPT(p, b)      esp_pixel_brightness565(p, b)

#ifdef __cplusplus
}
#endif

#endif
