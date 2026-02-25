#include "rg_system.h"
#include "rg_font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "FONT"

static rg_external_font_t external_fonts[RG_FONT_MAX_EXTERNAL];
static int external_font_count = 0;

void rg_font_init(void)
{
    memset(external_fonts, 0, sizeof(external_fonts));
    external_font_count = 0;
    RG_LOGI("Font module initialized");
}

void rg_font_deinit(void)
{
    rg_font_free_external();
}

static bool load_font_from_buffer(const uint8_t *buffer, size_t size)
{
    if (size < FONT_HEADER_SIZE)
    {
        RG_LOGE("Font data too small: %d bytes", (int)size);
        return false;
    }

    if (external_font_count >= RG_FONT_MAX_EXTERNAL)
    {
        RG_LOGE("Maximum external fonts reached");
        return false;
    }

    const uint8_t *ptr = buffer;

    char name[17];
    memcpy(name, ptr, 16);
    name[16] = '\0';
    ptr += 16;

    uint8_t type = *ptr++;
    uint8_t width = *ptr++;
    uint8_t height = *ptr++;
    ptr++;
    uint32_t chars = *(uint32_t*)ptr; ptr += 4;

    size_t data_size = size - FONT_HEADER_SIZE;

    rg_font_t *font = (rg_font_t *)rg_alloc(sizeof(rg_font_t) + data_size, MEM_SLOW);
    if (!font)
    {
        RG_LOGE("Failed to allocate font memory");
        return false;
    }

    strncpy(font->name, name, sizeof(font->name) - 1);
    font->name[sizeof(font->name) - 1] = '\0';
    font->type = type;
    font->width = width;
    font->height = height;
    font->chars = chars;
    memcpy(font->data, ptr, data_size);

    rg_external_font_t *ext = &external_fonts[external_font_count];
    snprintf(ext->path, sizeof(ext->path), "memory");
    strncpy(ext->name, font->name, sizeof(ext->name) - 1);
    ext->height = font->height;
    ext->font_data = font;
    ext->loaded = true;

    external_font_count++;

    RG_LOGI("Loaded font: %s (height=%d, chars=%d, size=%d bytes)",
            font->name, font->height, (int)font->chars, (int)data_size);

    return true;
}

bool rg_font_load_from_memory(const uint8_t *buffer, size_t size)
{
    if (!buffer || size == 0)
    {
        RG_LOGE("Invalid font buffer");
        return false;
    }

    return load_font_from_buffer(buffer, size);
}

const rg_font_t *rg_font_get(int index)
{
    if (index < 0)
        return NULL;

    if (index < external_font_count && external_fonts[index].loaded)
    {
        return external_fonts[index].font_data;
    }

    return NULL;
}

int rg_font_get_count(void)
{
    return external_font_count;
}

const char *rg_font_get_name(int index)
{
    if (index < 0 || index >= external_font_count)
        return NULL;

    return external_fonts[index].name;
}

void rg_font_free_external(void)
{
    for (int i = 0; i < external_font_count; i++)
    {
        if (external_fonts[i].font_data)
        {
            free(external_fonts[i].font_data);
            external_fonts[i].font_data = NULL;
            external_fonts[i].loaded = false;
        }
    }
    external_font_count = 0;
    RG_LOGI("Freed all external fonts");
}

const rg_font_t *rg_font_find_by_name(const char *name)
{
    if (!name)
        return NULL;

    for (int i = 0; i < external_font_count; i++)
    {
        if (external_fonts[i].loaded && 
            strncmp(external_fonts[i].name, name, sizeof(external_fonts[i].name)) == 0)
        {
            return external_fonts[i].font_data;
        }
    }
    return NULL;
}
