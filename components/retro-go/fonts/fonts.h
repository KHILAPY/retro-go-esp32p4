#pragma once

#include "../rg_gui.h"
#include "../rg_font.h"

/**
 * This file defines the built-in fonts for retro-go.
 *
 * LXGWWenKaiLite is a subset font containing:
 * - All ASCII characters (32-126)
 * - ~664 most common Chinese characters
 *
 * To regenerate the subset font:
 * python tools/convert_font.py --font tools/fonts/LXGWWenKai-Regular.ttf --sizes 14 --output components/retro-go/fonts --subset
 */

extern const rg_font_t font_basic8x8;
extern const rg_font_t font_DejaVu12;
extern const rg_font_t font_DejaVu15;
extern const rg_font_t font_VeraBold11;
extern const rg_font_t font_VeraBold14;
extern const rg_font_t font_LXGWWenKaiLite14;
extern const rg_font_t font_LXGWWenKaiLite18;

enum {
    RG_FONT_BASIC_8,
    RG_FONT_BASIC_12,
    RG_FONT_BASIC_16,
    RG_FONT_DEJAVU_12,
    RG_FONT_DEJAVU_15,
    RG_FONT_VERA_11,
    RG_FONT_VERA_14,
    RG_FONT_LXGW_LITE,
    RG_FONT_LXGW_LITE_18,
    RG_FONT_BUILTIN_MAX,
};

#define RG_FONT_MAX (RG_FONT_BUILTIN_MAX + RG_FONT_MAX_EXTERNAL)

static inline const rg_font_t *rg_font_get_builtin(int index)
{
    static const rg_font_t *builtin_fonts[RG_FONT_BUILTIN_MAX] = {
        &font_basic8x8,
        &font_basic8x8,
        &font_basic8x8,
        &font_DejaVu12,
        &font_DejaVu15,
        &font_VeraBold11,
        &font_VeraBold14,
        &font_LXGWWenKaiLite14,
        &font_LXGWWenKaiLite18,
    };

    if (index < 0 || index >= RG_FONT_BUILTIN_MAX)
        return NULL;

    return builtin_fonts[index];
}

static inline const rg_font_t *rg_font_get_combined(int index)
{
    if (index < 0)
        return NULL;

    if (index < RG_FONT_BUILTIN_MAX)
    {
        return rg_font_get_builtin(index);
    }
    else
    {
        return rg_font_get(index - RG_FONT_BUILTIN_MAX);
    }
}

static inline int rg_font_get_combined_count(void)
{
    return RG_FONT_BUILTIN_MAX + rg_font_get_count();
}

static inline const char *rg_font_get_combined_name(int index)
{
    if (index < 0)
        return NULL;

    if (index < RG_FONT_BUILTIN_MAX)
    {
        const rg_font_t *font = rg_font_get_builtin(index);
        return font ? font->name : NULL;
    }
    else
    {
        return rg_font_get_name(index - RG_FONT_BUILTIN_MAX);
    }
}
