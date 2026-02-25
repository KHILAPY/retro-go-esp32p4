/*****************************************************************************
    Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
    For further information, consult the LICENSE file in the root directory.
******************************************************************************/

/* S-DD1 decompressor
 *
 * Based on code and documentation by Andreas Naive, who deserves a great deal
 * of thanks and credit for figuring this out.
 *
 * Andreas says:
 * The author is greatly indebted with The Dumper, without whose help and
 * patience providing him with real S-DD1 data the research had never been
 * possible. He also wish to note that in the very beggining of his research,
 * Neviksti had done some steps in the right direction. By last, the author is
 * indirectly indebted to all the people that worked and contributed in the
 * S-DD1 issue in the past.
 *
 * Ported to pure C for ESP32-P4 by retro-go team.
 */

#include "sdd1emu.h"
#include "port.h"
#include <string.h>

static int valid_bits;
static uint16_t in_stream;
static uint8_t *in_buf;
static uint8_t bit_ctr[8];
static uint8_t context_states[32];
static int context_MPS[32];
static int bitplane_type;
static int high_context_bits;
static int low_context_bits;
static int prev_bits[8];

static const struct {
    uint8_t code_size;
    uint8_t MPS_next;
    uint8_t LPS_next;
} evolution_table[] = {
    { 0,25,25},
    { 0, 2, 1},
    { 0, 3, 1},
    { 0, 4, 2},
    { 0, 5, 3},
    { 1, 6, 4},
    { 1, 7, 5},
    { 1, 8, 6},
    { 1, 9, 7},
    { 2,10, 8},
    { 2,11, 9},
    { 2,12,10},
    { 2,13,11},
    { 3,14,12},
    { 3,15,13},
    { 3,16,14},
    { 3,17,15},
    { 4,18,16},
    { 4,19,17},
    { 5,20,18},
    { 5,21,19},
    { 6,22,20},
    { 6,23,21},
    { 7,24,22},
    { 7,24,23},
    { 0,26, 1},
    { 1,27, 2},
    { 2,28, 4},
    { 3,29, 8},
    { 4,30,12},
    { 5,31,16},
    { 6,32,18},
    { 7,24,22}
};

static const uint8_t run_table[128] = {
    128,  64,  96,  32, 112,  48,  80,  16, 120,  56,  88,  24, 104,  40,  72,
      8, 124,  60,  92,  28, 108,  44,  76,  12, 116,  52,  84,  20, 100,  36,
     68,   4, 126,  62,  94,  30, 110,  46,  78,  14, 118,  54,  86,  22, 102,
     38,  70,   6, 122,  58,  90,  26, 106,  42,  74,  10, 114,  50,  82,  18,
     98,  34,  66,   2, 127,  63,  95,  31, 111,  47,  79,  15, 119,  55,  87,
     23, 103,  39,  71,   7, 123,  59,  91,  27, 107,  43,  75,  11, 115,  51,
     83,  19,  99,  35,  67,   3, 125,  61,  93,  29, 109,  45,  77,  13, 117,
     53,  85,  21, 101,  37,  69,   5, 121,  57,  89,  25, 105,  41,  73,   9,
    113,  49,  81,  17,  97,  33,  65,   1
};

static inline uint8_t GetCodeword(int bits)
{
    uint8_t tmp;

    if (!valid_bits) {
        in_stream |= *(in_buf++);
        valid_bits = 8;
    }
    in_stream <<= 1;
    valid_bits--;
    in_stream ^= 0x8000;
    if (in_stream & 0x8000) return 0x80 + (1 << bits);
    tmp = (in_stream >> 8) | (0x7f >> bits);
    in_stream <<= bits;
    valid_bits -= bits;
    if (valid_bits < 0) {
        in_stream |= (*(in_buf++)) << (-valid_bits);
        valid_bits += 8;
    }
    return run_table[tmp];
}

static inline uint8_t GolombGetBit(int code_size)
{
    if (!bit_ctr[code_size]) bit_ctr[code_size] = GetCodeword(code_size);
    bit_ctr[code_size]--;
    if (bit_ctr[code_size] == 0x80) {
        bit_ctr[code_size] = 0;
        return 2;
    }
    return (bit_ctr[code_size] == 0) ? 1 : 0;
}

static inline uint8_t ProbGetBit(uint8_t context)
{
    uint8_t state = context_states[context];
    uint8_t bit = GolombGetBit(evolution_table[state].code_size);

    if (bit & 1) {
        context_states[context] = evolution_table[state].LPS_next;
        if (state < 2) {
            context_MPS[context] ^= 1;
            return context_MPS[context];
        } else {
            return context_MPS[context] ^ 1;
        }
    } else if (bit) {
        context_states[context] = evolution_table[state].MPS_next;
    }
    return context_MPS[context];
}

static inline uint8_t GetBit(uint8_t cur_bitplane)
{
    uint8_t bit;

    bit = ProbGetBit(((cur_bitplane & 1) << 4)
                   | ((prev_bits[cur_bitplane] & high_context_bits) >> 5)
                   | (prev_bits[cur_bitplane] & low_context_bits));

    prev_bits[cur_bitplane] <<= 1;
    prev_bits[cur_bitplane] |= bit;
    return bit;
}

#ifdef CONFIG_IDF_TARGET_ESP32P4
IRAM_ATTR void SDD1_decompress(uint8_t *out, uint8_t *in, int len)
#else
void SDD1_decompress(uint8_t *out, uint8_t *in, int len)
#endif
{
    uint8_t bit, i, plane;
    uint8_t byte1, byte2;

    if (len == 0) len = 0x10000;

    bitplane_type = in[0] >> 6;

    switch (in[0] & 0x30) {
      case 0x00:
        high_context_bits = 0x01c0;
        low_context_bits  = 0x0001;
        break;
      case 0x10:
        high_context_bits = 0x0180;
        low_context_bits  = 0x0001;
        break;
      case 0x20:
        high_context_bits = 0x00c0;
        low_context_bits  = 0x0001;
        break;
      case 0x30:
        high_context_bits = 0x0180;
        low_context_bits  = 0x0003;
        break;
    }

    in_stream = (in[0] << 11) | (in[1] << 3);
    valid_bits = 5;
    in_buf = in + 2;
    memset(bit_ctr, 0, sizeof(bit_ctr));
    memset(context_states, 0, sizeof(context_states));
    memset(context_MPS, 0, sizeof(context_MPS));
    memset(prev_bits, 0, sizeof(prev_bits));

    switch (bitplane_type) {
      case 0:
        while (1) {
            for (byte1 = byte2 = 0, bit = 0x80; bit; bit >>= 1) {
                if (GetBit(0)) byte1 |= bit;
                if (GetBit(1)) byte2 |= bit;
            }
            *(out++) = byte1;
            if (!--len) return;
            *(out++) = byte2;
            if (!--len) return;
        }
        break;
      case 1:
        i = plane = 0;
        while (1) {
            for (byte1 = byte2 = 0, bit = 0x80; bit; bit >>= 1) {
                if (GetBit(plane)) byte1 |= bit;
                if (GetBit(plane + 1)) byte2 |= bit;
            }
            *(out++) = byte1;
            if (!--len) return;
            *(out++) = byte2;
            if (!--len) return;
            if (!(i += 32)) plane = (plane + 2) & 7;
        }
        break;
      case 2:
        i = plane = 0;
        while (1) {
            for (byte1 = byte2 = 0, bit = 0x80; bit; bit >>= 1) {
                if (GetBit(plane)) byte1 |= bit;
                if (GetBit(plane + 1)) byte2 |= bit;
            }
            *(out++) = byte1;
            if (!--len) return;
            *(out++) = byte2;
            if (!--len) return;
            if (!(i += 32)) plane ^= 2;
        }
        break;
      case 3:
        do {
            for (byte1 = plane = 0, bit = 1; bit; bit <<= 1, plane++) {
                if (GetBit(plane)) byte1 |= bit;
            }
            *(out++) = byte1;
        } while (--len);
        break;
    }
}
