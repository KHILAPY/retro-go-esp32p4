/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

void fx_color (void)
{
    uint8_t c = (uint8_t) SREG;

    if (GSU.vPlotOptionReg & 0x04)
        c = (c & 0xf0) | (c >> 4);
    if (GSU.vPlotOptionReg & 0x08)
    {
        GSU.vColorReg &= 0xf0;
        GSU.vColorReg |= c & 0x0f;
    }
    else
        GSU.vColorReg = USEX8(c);

    CLRFLAGS;
    R15++;
}

void fx_cmode (void)
{
    GSU.vPlotOptionReg = SREG;

    if (GSU.vPlotOptionReg & 0x10)
        GSU.vScreenHeight = 256;
    else
        GSU.vScreenHeight = GSU.vScreenRealHeight;

    CLRFLAGS;
    R15++;
}

void fx_getc (void)
{
    DREG = COLR;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_getb (void)
{
    DREG = GSU.vRomBuffer;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    CLRFLAGS;
    R15++;
}

void fx_getbh (void)
{
    DREG = (GSU.vRomBuffer >> 4) & 0x0f;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    CLRFLAGS;
    R15++;
}

void fx_getbl (void)
{
    DREG = GSU.vRomBuffer & 0x0f;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    CLRFLAGS;
    R15++;
}

void fx_getbs (void)
{
    DREG = SEX8(GSU.vRomBuffer);
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    CLRFLAGS;
    R15++;
}

void fx_plot_2bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v, c;

    R15++;
    CLRFLAGS;
    R1++;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    if (!(GSU.vPlotOptionReg & 0x01) && !(COLR & 0xf))
        return;

    if (GSU.vPlotOptionReg & 0x02)
        c = ((x ^ y) & 1) ? (uint8_t)(GSU.vColorReg >> 4) : (uint8_t)GSU.vColorReg;
    else
        c = (uint8_t)GSU.vColorReg;

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    if (c & 0x01)
        a[0] |= v;
    else
        a[0] &= ~v;

    if (c & 0x02)
        a[1] |= v;
    else
        a[1] &= ~v;
}

void fx_rpix_2bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v;

    R15++;
    CLRFLAGS;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    DREG = 0;
    DREG |= ((uint32_t)((a[0] & v) != 0)) << 0;
    DREG |= ((uint32_t)((a[1] & v) != 0)) << 1;
    TESTR14;
}

void fx_plot_4bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v, c;

    R15++;
    CLRFLAGS;
    R1++;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    if (!(GSU.vPlotOptionReg & 0x01) && !(COLR & 0xf))
        return;

    if (GSU.vPlotOptionReg & 0x02)
        c = ((x ^ y) & 1) ? (uint8_t)(GSU.vColorReg >> 4) : (uint8_t)GSU.vColorReg;
    else
        c = (uint8_t)GSU.vColorReg;

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    if (c & 0x01)
        a[0x00] |= v;
    else
        a[0x00] &= ~v;

    if (c & 0x02)
        a[0x01] |= v;
    else
        a[0x01] &= ~v;

    if (c & 0x04)
        a[0x10] |= v;
    else
        a[0x10] &= ~v;

    if (c & 0x08)
        a[0x11] |= v;
    else
        a[0x11] &= ~v;
}

void fx_rpix_4bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v;

    R15++;
    CLRFLAGS;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    DREG = 0;
    DREG |= ((uint32_t)((a[0x00] & v) != 0)) << 0;
    DREG |= ((uint32_t)((a[0x01] & v) != 0)) << 1;
    DREG |= ((uint32_t)((a[0x10] & v) != 0)) << 2;
    DREG |= ((uint32_t)((a[0x11] & v) != 0)) << 3;
    TESTR14;
}

void fx_plot_8bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v, c;

    R15++;
    CLRFLAGS;
    R1++;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    c = (uint8_t)GSU.vColorReg;
    if (!(GSU.vPlotOptionReg & 0x10))
    {
        if (!(GSU.vPlotOptionReg & 0x01) && (!c || ((GSU.vPlotOptionReg & 0x08) && !(c & 0xf))))
            return;
    }
    else
    if (!(GSU.vPlotOptionReg & 0x01) && !c)
        return;

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    if (c & 0x01)
        a[0x00] |= v;
    else
        a[0x00] &= ~v;

    if (c & 0x02)
        a[0x01] |= v;
    else
        a[0x01] &= ~v;

    if (c & 0x04)
        a[0x10] |= v;
    else
        a[0x10] &= ~v;

    if (c & 0x08)
        a[0x11] |= v;
    else
        a[0x11] &= ~v;

    if (c & 0x10)
        a[0x20] |= v;
    else
        a[0x20] &= ~v;

    if (c & 0x20)
        a[0x21] |= v;
    else
        a[0x21] &= ~v;

    if (c & 0x40)
        a[0x30] |= v;
    else
        a[0x30] &= ~v;

    if (c & 0x80)
        a[0x31] |= v;
    else
        a[0x31] &= ~v;
}

void fx_rpix_8bit (void)
{
    uint32_t x = USEX8(R1);
    uint32_t y = USEX8(R2);
    uint8_t *a;
    uint8_t v;

    R15++;
    CLRFLAGS;

#ifdef CHECK_LIMITS
    if (y >= GSU.vScreenHeight)
        return;
#endif

    a = GSU.apvScreen[y >> 3] + GSU.x[x >> 3] + ((y & 7) << 1);
    v = 128 >> (x & 7);

    DREG = 0;
    DREG |= ((uint32_t)((a[0x00] & v) != 0)) << 0;
    DREG |= ((uint32_t)((a[0x01] & v) != 0)) << 1;
    DREG |= ((uint32_t)((a[0x10] & v) != 0)) << 2;
    DREG |= ((uint32_t)((a[0x11] & v) != 0)) << 3;
    DREG |= ((uint32_t)((a[0x20] & v) != 0)) << 4;
    DREG |= ((uint32_t)((a[0x21] & v) != 0)) << 5;
    DREG |= ((uint32_t)((a[0x30] & v) != 0)) << 6;
    DREG |= ((uint32_t)((a[0x31] & v) != 0)) << 7;
    GSU.vZero = DREG;
    TESTR14;
}

void fx_plot_obj (void)
{
}

void fx_rpix_obj (void)
{
}

void (*fx_PlotTable[]) (void) =
{
    &fx_plot_2bit, &fx_plot_4bit, &fx_plot_4bit, &fx_plot_8bit, &fx_plot_obj,
    &fx_rpix_2bit, &fx_rpix_4bit, &fx_rpix_4bit, &fx_rpix_8bit, &fx_rpix_obj
};
