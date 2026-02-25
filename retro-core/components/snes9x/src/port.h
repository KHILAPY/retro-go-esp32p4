/* This file is part of Snes9x. See LICENSE file. */

#ifndef _PORT_H_
#define _PORT_H_

#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#ifdef RETRO_GO
#include <rg_system.h>
// ESP-IDF 定义了 BIT8, BIT16 宏，与 snes9x 函数名冲突
#ifdef BIT8
#undef BIT8
#endif
#ifdef BIT16
#undef BIT16
#endif
#endif

#ifndef INLINE
#define INLINE inline
#endif

#define PIXEL_FORMAT RGB565

#include "pixform.h"

#define SLASH_STR "/"
#define SLASH_CHAR '/'

#define ABS(X)   ((X) <  0  ? -(X) : (X))
#ifndef MIN
#define MIN(A,B) ((A) < (B) ?  (A) : (B))
#endif
#ifndef MAX
#define MAX(A,B) ((A) > (B) ?  (A) : (B))
#endif

static INLINE int32_t _isqrt(int32_t val)
{
   int32_t squaredbit, remainder, root;

   if (val < 1)
      return 0;

   squaredbit  = 1 << 30;
   remainder = val;
   root = 0;

   while (squaredbit > 0)
   {
      if (remainder >= (squaredbit | root))
      {
         remainder -= (squaredbit | root);
         root >>= 1;
         root |= squaredbit;
      }
      else
         root >>= 1;
      squaredbit >>= 2;
   }

   return root;
}
#endif
