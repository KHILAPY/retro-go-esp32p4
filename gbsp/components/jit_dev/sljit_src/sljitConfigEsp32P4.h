/*
 * SLJIT Configuration for ESP32-P4 (RISC-V 32-bit)
 * 
 * This file provides custom configuration for the SLJIT library
 * to work with ESP32-P4's RISC-V processor and ESP-IDF environment.
 */

#ifndef SLJIT_CONFIG_ESP32P4_H_
#define SLJIT_CONFIG_ESP32P4_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Indicate that we have a pre-config header - must be before sljitLir.h */
#ifndef SLJIT_HAVE_CONFIG_PRE
#define SLJIT_HAVE_CONFIG_PRE 1
#endif

/* --------------------------------------------------------------------- */
/*  Target CPU Configuration                                             */
/* --------------------------------------------------------------------- */

/* ESP32-P4 uses RISC-V 32-bit processor */
/* Unconditionally define RISCV_32 since this config is only used for ESP32-P4 */
#define SLJIT_CONFIG_RISCV_32 1

/* Enable RISC-V compressed instructions (RVC) - ESP32-P4 supports this */
#ifdef __riscv_compressed
    #define SLJIT_CONFIG_RISCV_COMPRESSED 200
#endif

/* Enable RISC-V Bit Manipulation (B extension) - Zbb subset */
#ifdef __riscv_zbb
    #define SLJIT_CONFIG_RISCV_BITMANIP_B 93
#endif

/* Enable RISC-V Bit Manipulation (A extension) - Zba subset */
#ifdef __riscv_zba
    #define SLJIT_CONFIG_RISCV_BITMANIP_A 93
#endif

/* --------------------------------------------------------------------- */
/*  ESP-IDF Memory Allocation                                            */
/* --------------------------------------------------------------------- */

#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

/* Use ESP-IDF's heap_caps_malloc for executable memory */
/* ESP32-P4: IRAM is fully used by firmware, use PSRAM with XIP support */
/* ESP32-P4 supports Execute-In-Place from PSRAM (CONFIG_SOC_SPIRAM_XIP_SUPPORTED) */
#ifndef SLJIT_MALLOC_EXEC
#define SLJIT_MALLOC_EXEC(size, allocator_data) \
    heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#endif

#ifndef SLJIT_FREE_EXEC
#define SLJIT_FREE_EXEC(ptr, allocator_data) \
    heap_caps_free(ptr)
#endif

/* Regular memory allocation */
#ifndef SLJIT_MALLOC
#define SLJIT_MALLOC(size, allocator_data) \
    heap_caps_malloc(size, MALLOC_CAP_8BIT)
#endif

#ifndef SLJIT_FREE
#define SLJIT_FREE(ptr, allocator_data) \
    heap_caps_free(ptr)
#endif

/* Memory operations */
#ifndef SLJIT_MEMCPY
#define SLJIT_MEMCPY(dest, src, size) memcpy(dest, src, size)
#endif

#ifndef SLJIT_MEMMOVE
#define SLJIT_MEMMOVE(dest, src, size) memmove(dest, src, size)
#endif

#ifndef SLJIT_MEMSET
#define SLJIT_MEMSET(dest, value, size) memset(dest, value, size)
#endif

/* --------------------------------------------------------------------- */
/*  Debug Configuration                                                  */
/* --------------------------------------------------------------------- */

/* Enable debug in development, disable for production */
#ifndef SLJIT_DEBUG
#define SLJIT_DEBUG 0
#endif

/* Disable verbose for production */
#ifndef SLJIT_VERBOSE
#define SLJIT_VERBOSE 0
#endif

/* Enable argument checks for debugging */
#ifndef SLJIT_ARGUMENT_CHECKS
#define SLJIT_ARGUMENT_CHECKS 0
#endif

/* --------------------------------------------------------------------- */
/*  Threading Configuration                                              */
/* --------------------------------------------------------------------- */

/* ESP32-P4 is dual-core, but GBA emulation is single-threaded */
#ifndef SLJIT_SINGLE_THREADED
#define SLJIT_SINGLE_THREADED 1
#endif

/* --------------------------------------------------------------------- */
/*  Utility Configuration                                                */
/* --------------------------------------------------------------------- */

/* Enable utility stack */
#ifndef SLJIT_UTIL_STACK
#define SLJIT_UTIL_STACK 1
#endif

/* Use simple stack allocation */
#ifndef SLJIT_UTIL_SIMPLE_STACK_ALLOCATION
#define SLJIT_UTIL_SIMPLE_STACK_ALLOCATION 1
#endif

/* --------------------------------------------------------------------- */
/*  Executable Allocator Configuration                                   */
/* --------------------------------------------------------------------- */

/* Use our custom allocator */
#ifndef SLJIT_EXECUTABLE_ALLOCATOR
#define SLJIT_EXECUTABLE_ALLOCATOR 0
#endif

/* Disable protected/WX allocators - we handle this ourselves */
#ifndef SLJIT_PROT_EXECUTABLE_ALLOCATOR
#define SLJIT_PROT_EXECUTABLE_ALLOCATOR 0
#endif

#ifndef SLJIT_WX_EXECUTABLE_ALLOCATOR
#define SLJIT_WX_EXECUTABLE_ALLOCATOR 0
#endif

/* --------------------------------------------------------------------- */
/*  JIT-Specific Configuration                                           */
/* --------------------------------------------------------------------- */

/* Cache line size for ESP32-P4 */
#define GBA_JIT_CACHE_LINE_SIZE 64

/* ROM translation cache size - using PSRAM which has plenty of space */
#define GBA_JIT_ROM_CACHE_SIZE (256 * 1024)

/* RAM translation cache size - using PSRAM which has plenty of space */
#define GBA_JIT_RAM_CACHE_SIZE (64 * 1024)

/* Block hash table size */
#define GBA_JIT_BLOCK_HASH_SIZE 4096

/* Maximum instructions per block */
#define GBA_JIT_MAX_BLOCK_INSTRUCTIONS 64

/* Enable JIT statistics */
#define GBA_JIT_ENABLE_STATS 1

/* Enable JIT debug logging */
#define GBA_JIT_DEBUG_LOG 1

#if GBA_JIT_DEBUG_LOG
#define GBA_JIT_LOGI(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define GBA_JIT_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define GBA_JIT_LOGW(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#else
#define GBA_JIT_LOGI(tag, fmt, ...)
#define GBA_JIT_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define GBA_JIT_LOGW(tag, fmt, ...)
#endif

/* --------------------------------------------------------------------- */
/*  Abort Function                                                       */
/* --------------------------------------------------------------------- */

#ifndef SLJIT_ABORT
#define SLJIT_ABORT() abort()
#endif

#ifdef __cplusplus
}
#endif

#endif /* SLJIT_CONFIG_ESP32P4_H_ */
