/*
 *    Stack-less Just-In-Time compiler
 *
 *    Copyright Zoltan Herczeg (hzmester@freemail.hu). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER(S) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SLJIT Internal Configuration for ESP32-P4 (RISC-V 32-bit only)
 * 
 * 精简版：仅保留 RISC-V 32位架构支持
 */

#ifndef SLJIT_CONFIG_INTERNAL_H_
#define SLJIT_CONFIG_INTERNAL_H_

/*
   SLJIT defines the following architecture dependent types and macros:

   Types:
     sljit_s8, sljit_u8   : signed and unsigned 8 bit integer type
     sljit_s16, sljit_u16 : signed and unsigned 16 bit integer type
     sljit_s32, sljit_u32 : signed and unsigned 32 bit integer type
     sljit_sw, sljit_uw   : signed and unsigned machine word, enough to store a pointer
     sljit_sp, sljit_up   : signed and unsigned pointer value
     sljit_f32            : 32 bit single precision floating point value
     sljit_f64            : 64 bit double precision floating point value

   Macros for feature detection (boolean):
     SLJIT_32BIT_ARCHITECTURE : 32 bit architecture
     SLJIT_LITTLE_ENDIAN      : little endian architecture
     SLJIT_UNALIGNED          : unaligned memory accesses supported
     SLJIT_FPU_UNALIGNED      : unaligned fpu memory accesses supported
     SLJIT_MASKED_SHIFT       : all word shifts are always masked
     SLJIT_MASKED_SHIFT32     : all 32 bit shifts are always masked
     SLJIT_UPPER_BITS_IGNORED : 32 bit operations ignores upper bits
     SLJIT_UPPER_BITS_SIGN_EXTENDED : sign bit replicated in upper bits
     SLJIT_SHARED_COMPARISON_FLAGS : signed/unsigned comparisons set same flags

   Constants:
     SLJIT_NUMBER_OF_REGISTERS           : number of available registers
     SLJIT_NUMBER_OF_SCRATCH_REGISTERS   : number of scratch registers
     SLJIT_NUMBER_OF_SAVED_REGISTERS     : number of saved registers
     SLJIT_NUMBER_OF_FLOAT_REGISTERS     : number of float registers
     SLJIT_NUMBER_OF_VECTOR_REGISTERS    : number of vector registers
     SLJIT_WORD_SHIFT                    : shift for word array access
     SLJIT_LOCALS_OFFSET                 : local space starting offset
*/

#if (defined SLJIT_VERBOSE && SLJIT_VERBOSE) \
	|| (defined SLJIT_DEBUG && SLJIT_DEBUG && (!defined(SLJIT_ASSERT) || !defined(SLJIT_UNREACHABLE)))
#include <stdio.h>
#endif

#if (defined SLJIT_DEBUG && SLJIT_DEBUG \
	&& (!defined(SLJIT_ASSERT) || !defined(SLJIT_UNREACHABLE) || !defined(SLJIT_HALT_PROCESS)))
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**********************************/
/* External function definitions. */
/**********************************/

#ifndef SLJIT_MALLOC
#define SLJIT_MALLOC(size, allocator_data) (malloc(size))
#endif

#ifndef SLJIT_FREE
#define SLJIT_FREE(ptr, allocator_data) (free(ptr))
#endif

#ifndef SLJIT_MEMCPY
#define SLJIT_MEMCPY(dest, src, len) (memcpy(dest, src, len))
#endif

#ifndef SLJIT_MEMMOVE
#define SLJIT_MEMMOVE(dest, src, len) (memmove(dest, src, len))
#endif

#ifndef SLJIT_ZEROMEM
#define SLJIT_ZEROMEM(dest, len) (memset(dest, 0, len))
#endif

/***************************/
/* Compiler helper macros. */
/***************************/

#if !defined(SLJIT_LIKELY) && !defined(SLJIT_UNLIKELY)

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define SLJIT_LIKELY(x)		__builtin_expect((x), 1)
#define SLJIT_UNLIKELY(x)	__builtin_expect((x), 0)
#else
#define SLJIT_LIKELY(x)		(x)
#define SLJIT_UNLIKELY(x)	(x)
#endif

#endif /* !defined(SLJIT_LIKELY) && !defined(SLJIT_UNLIKELY) */

#ifndef SLJIT_INLINE
#define SLJIT_INLINE __inline
#endif /* !SLJIT_INLINE */

#ifndef SLJIT_NOINLINE
#if defined(__GNUC__)
#define SLJIT_NOINLINE __attribute__ ((noinline))
#else
#define SLJIT_NOINLINE
#endif
#endif /* !SLJIT_NOINLINE */

#ifndef SLJIT_UNUSED_ARG
#define SLJIT_UNUSED_ARG(arg) (void)arg
#endif

/*********************************/
/* Type of public API functions. */
/*********************************/

#ifndef SLJIT_API_FUNC_ATTRIBUTE
#if (defined SLJIT_CONFIG_STATIC && SLJIT_CONFIG_STATIC)
#if defined(__GNUC__)
#define SLJIT_API_FUNC_ATTRIBUTE static __attribute__((unused))
#else
#define SLJIT_API_FUNC_ATTRIBUTE static
#endif
#else
#define SLJIT_API_FUNC_ATTRIBUTE
#endif /* SLJIT_CONFIG_STATIC */
#endif /* SLJIT_API_FUNC_ATTRIBUTE */

/****************************/
/* Instruction cache flush. */
/****************************/

/* RISC-V 使用 GCC builtin 或 __clear_cache */
#ifndef SLJIT_CACHE_FLUSH
#if defined(__GNUC__) || defined(__clang__)
#define SLJIT_CACHE_FLUSH(from, to) \
	__builtin___clear_cache((char*)(from), (char*)(to))
#else
#define SLJIT_CACHE_FLUSH(from, to) \
	__clear_cache((char*)(from), (char*)(to))
#endif
#endif /* !SLJIT_CACHE_FLUSH */

/******************************************************/
/*    Integer and floating point type definitions.    */
/******************************************************/

/* 8 bit byte type. */
typedef unsigned char sljit_u8;
typedef signed char sljit_s8;

/* 16 bit half-word type. */
typedef unsigned short int sljit_u16;
typedef signed short int sljit_s16;

/* 32 bit integer type. */
typedef unsigned int sljit_u32;
typedef signed int sljit_s32;

/* RISC-V 32-bit: machine word is 32 bit */
#define SLJIT_32BIT_ARCHITECTURE 1
#define SLJIT_WORD_SHIFT 2
typedef unsigned int sljit_uw;
typedef int sljit_sw;

typedef sljit_sw sljit_sp;
typedef sljit_uw sljit_up;

/* Floating point types. */
typedef float sljit_f32;
typedef double sljit_f64;

/* Shift for pointer sized data. */
#define SLJIT_POINTER_SHIFT SLJIT_WORD_SHIFT

/* Shift for floating point sized data. */
#define SLJIT_F32_SHIFT 2
#define SLJIT_F64_SHIFT 3

#define SLJIT_CONV_RESULT_MAX_INT 0
#define SLJIT_CONV_RESULT_MIN_INT 1
#define SLJIT_CONV_RESULT_ZERO 2

/* RISC-V float to int conversion results */
#define SLJIT_CONV_MAX_FLOAT SLJIT_CONV_RESULT_MAX_INT
#define SLJIT_CONV_MIN_FLOAT SLJIT_CONV_RESULT_MIN_INT
#define SLJIT_CONV_NAN_FLOAT SLJIT_CONV_RESULT_MAX_INT

#ifndef SLJIT_W
#define SLJIT_W(w)	(w)
#endif /* !SLJIT_W */

/*************************/
/* Endianness detection. */
/*************************/

/* RISC-V is little endian */
#ifndef SLJIT_LITTLE_ENDIAN
#define SLJIT_LITTLE_ENDIAN 1
#endif

#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN) && (defined SLJIT_LITTLE_ENDIAN && SLJIT_LITTLE_ENDIAN)
#error "Exactly one endianness must be selected"
#endif

/* RISC-V supports unaligned memory access */
#ifndef SLJIT_UNALIGNED
#define SLJIT_UNALIGNED 1
#endif

#ifndef SLJIT_FPU_UNALIGNED
#define SLJIT_FPU_UNALIGNED 1
#endif

/*****************************************************************************************/
/* Calling convention of functions generated by SLJIT or called from the generated code. */
/*****************************************************************************************/

#ifndef SLJIT_FUNC
#define SLJIT_FUNC
#endif /* !SLJIT_FUNC */

#ifndef SLJIT_RETURN_ADDRESS_OFFSET
#define SLJIT_RETURN_ADDRESS_OFFSET 0
#endif /* SLJIT_RETURN_ADDRESS_OFFSET */

/***************************************************/
/* Functions of the built-in executable allocator. */
/***************************************************/

#if (defined SLJIT_EXECUTABLE_ALLOCATOR && SLJIT_EXECUTABLE_ALLOCATOR)
SLJIT_API_FUNC_ATTRIBUTE void* sljit_malloc_exec(sljit_uw size);
SLJIT_API_FUNC_ATTRIBUTE void sljit_free_exec(void* ptr);
#define SLJIT_BUILTIN_MALLOC_EXEC(size, exec_allocator_data) sljit_malloc_exec(size)
#define SLJIT_BUILTIN_FREE_EXEC(ptr, exec_allocator_data) sljit_free_exec(ptr)

#ifndef SLJIT_MALLOC_EXEC
#define SLJIT_MALLOC_EXEC(size, exec_allocator_data) SLJIT_BUILTIN_MALLOC_EXEC((size), (exec_allocator_data))
#endif /* SLJIT_MALLOC_EXEC */

#ifndef SLJIT_FREE_EXEC
#define SLJIT_FREE_EXEC(ptr, exec_allocator_data) SLJIT_BUILTIN_FREE_EXEC((ptr), (exec_allocator_data))
#endif /* SLJIT_FREE_EXEC */

#endif /* SLJIT_EXECUTABLE_ALLOCATOR */

#ifndef SLJIT_EXEC_OFFSET
#define SLJIT_EXEC_OFFSET(ptr) 0
#endif

/**********************************************/
/* Registers and locals offset determination. */
/**********************************************/

/* RISC-V 32-bit register configuration */
#define SLJIT_NUMBER_OF_REGISTERS 23
#define SLJIT_NUMBER_OF_SAVED_REGISTERS 12
#define SLJIT_NUMBER_OF_TEMPORARY_REGISTERS 5
#define SLJIT_NUMBER_OF_FLOAT_REGISTERS 30
#define SLJIT_NUMBER_OF_SAVED_FLOAT_REGISTERS 12
#define SLJIT_NUMBER_OF_TEMPORARY_FLOAT_REGISTERS 2
#define SLJIT_SEPARATE_VECTOR_REGISTERS 1
#define SLJIT_NUMBER_OF_VECTOR_REGISTERS 30
#define SLJIT_NUMBER_OF_SAVED_VECTOR_REGISTERS 0
#define SLJIT_NUMBER_OF_TEMPORARY_VECTOR_REGISTERS 2
#define SLJIT_TMP_DEST_REG SLJIT_TMP_R1
#define SLJIT_TMP_OPT_REG SLJIT_TMP_R0
#define SLJIT_TMP_FLAG_REG SLJIT_TMP_R3
#define SLJIT_TMP_DEST_FREG SLJIT_TMP_FR0
#define SLJIT_TMP_DEST_VREG SLJIT_TMP_VR0
#define SLJIT_LOCALS_OFFSET_BASE 0
#define SLJIT_MASKED_SHIFT 1
#define SLJIT_MASKED_SHIFT32 1
#define SLJIT_UPPER_BITS_IGNORED 1
#define SLJIT_UPPER_BITS_SIGN_EXTENDED 1
#define SLJIT_SHARED_COMPARISON_FLAGS 1

#define SLJIT_LOCALS_OFFSET (SLJIT_LOCALS_OFFSET_BASE)

#define SLJIT_NUMBER_OF_SCRATCH_REGISTERS \
	(SLJIT_NUMBER_OF_REGISTERS - SLJIT_NUMBER_OF_SAVED_REGISTERS)

#define SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS \
	(SLJIT_NUMBER_OF_FLOAT_REGISTERS - SLJIT_NUMBER_OF_SAVED_FLOAT_REGISTERS)

#define SLJIT_NUMBER_OF_SCRATCH_VECTOR_REGISTERS \
	(SLJIT_NUMBER_OF_VECTOR_REGISTERS - SLJIT_NUMBER_OF_SAVED_VECTOR_REGISTERS)

/**********************************/
/* Temporary register management. */
/**********************************/

#define SLJIT_TMP_REGISTER_BASE (SLJIT_NUMBER_OF_REGISTERS + 2)
#define SLJIT_TMP_FREGISTER_BASE (SLJIT_NUMBER_OF_FLOAT_REGISTERS + 1)
#define SLJIT_TMP_VREGISTER_BASE (SLJIT_NUMBER_OF_VECTOR_REGISTERS + 1)

/* Temporary registers */
#define SLJIT_TMP_R0		(SLJIT_TMP_REGISTER_BASE + 0)
#define SLJIT_TMP_R1		(SLJIT_TMP_REGISTER_BASE + 1)
#define SLJIT_TMP_R2		(SLJIT_TMP_REGISTER_BASE + 2)
#define SLJIT_TMP_R3		(SLJIT_TMP_REGISTER_BASE + 3)
#define SLJIT_TMP_R4		(SLJIT_TMP_REGISTER_BASE + 4)
#define SLJIT_TMP_R5		(SLJIT_TMP_REGISTER_BASE + 5)
#define SLJIT_TMP_R6		(SLJIT_TMP_REGISTER_BASE + 6)
#define SLJIT_TMP_R7		(SLJIT_TMP_REGISTER_BASE + 7)
#define SLJIT_TMP_R8		(SLJIT_TMP_REGISTER_BASE + 8)
#define SLJIT_TMP_R9		(SLJIT_TMP_REGISTER_BASE + 9)
#define SLJIT_TMP_R(i)		(SLJIT_TMP_REGISTER_BASE + (i))

#define SLJIT_TMP_FR0		(SLJIT_TMP_FREGISTER_BASE + 0)
#define SLJIT_TMP_FR1		(SLJIT_TMP_FREGISTER_BASE + 1)
#define SLJIT_TMP_FR2		(SLJIT_TMP_FREGISTER_BASE + 2)
#define SLJIT_TMP_FR3		(SLJIT_TMP_FREGISTER_BASE + 3)
#define SLJIT_TMP_FR4		(SLJIT_TMP_FREGISTER_BASE + 4)
#define SLJIT_TMP_FR5		(SLJIT_TMP_FREGISTER_BASE + 5)
#define SLJIT_TMP_FR6		(SLJIT_TMP_FREGISTER_BASE + 6)
#define SLJIT_TMP_FR7		(SLJIT_TMP_FREGISTER_BASE + 7)
#define SLJIT_TMP_FR8		(SLJIT_TMP_FREGISTER_BASE + 8)
#define SLJIT_TMP_FR9		(SLJIT_TMP_FREGISTER_BASE + 9)
#define SLJIT_TMP_FR(i)		(SLJIT_TMP_FREGISTER_BASE + (i))

#define SLJIT_TMP_VR0		(SLJIT_TMP_VREGISTER_BASE + 0)
#define SLJIT_TMP_VR1		(SLJIT_TMP_VREGISTER_BASE + 1)
#define SLJIT_TMP_VR2		(SLJIT_TMP_VREGISTER_BASE + 2)
#define SLJIT_TMP_VR3		(SLJIT_TMP_VREGISTER_BASE + 3)
#define SLJIT_TMP_VR4		(SLJIT_TMP_VREGISTER_BASE + 4)
#define SLJIT_TMP_VR5		(SLJIT_TMP_VREGISTER_BASE + 5)
#define SLJIT_TMP_VR6		(SLJIT_TMP_VREGISTER_BASE + 6)
#define SLJIT_TMP_VR7		(SLJIT_TMP_VREGISTER_BASE + 7)
#define SLJIT_TMP_VR8		(SLJIT_TMP_VREGISTER_BASE + 8)
#define SLJIT_TMP_VR9		(SLJIT_TMP_VREGISTER_BASE + 9)
#define SLJIT_TMP_VR(i)		(SLJIT_TMP_VREGISTER_BASE + (i))

/********************************/
/* CPU status flags management. */
/********************************/

/* RISC-V has status flags state */
#define SLJIT_HAS_STATUS_FLAGS_STATE 1

/*************************************/
/* Floating point register management. */
/*************************************/

/* RISC-V 32-bit: f64 uses single register (not paired f32) */
#define SLJIT_F64_SECOND(reg) \
	(reg)

/*************************************/
/* Debug and verbose related macros. */
/*************************************/

#if (defined SLJIT_DEBUG && SLJIT_DEBUG)

#if !defined(SLJIT_ASSERT) || !defined(SLJIT_UNREACHABLE)

#ifndef SLJIT_HALT_PROCESS
#define SLJIT_HALT_PROCESS() \
	abort();
#endif /* !SLJIT_HALT_PROCESS */

#endif /* !SLJIT_ASSERT || !SLJIT_UNREACHABLE */

#ifndef SLJIT_ASSERT

#define SLJIT_ASSERT(x) \
	do { \
		if (SLJIT_UNLIKELY(!(x))) { \
			printf("Assertion failed at " __FILE__ ":%d\n", __LINE__); \
			SLJIT_HALT_PROCESS(); \
		} \
	} while (0)

#endif /* !SLJIT_ASSERT */

#ifndef SLJIT_UNREACHABLE

#define SLJIT_UNREACHABLE() \
	do { \
		printf("Should never been reached " __FILE__ ":%d\n", __LINE__); \
		SLJIT_HALT_PROCESS(); \
	} while (0)

#endif /* !SLJIT_UNREACHABLE */

#else /* (defined SLJIT_DEBUG && SLJIT_DEBUG) */

#undef SLJIT_ASSERT
#undef SLJIT_UNREACHABLE

#define SLJIT_ASSERT(x) \
	do { } while (0)
#define SLJIT_UNREACHABLE() \
	do { } while (0)

#endif /* (defined SLJIT_DEBUG && SLJIT_DEBUG) */

#ifndef SLJIT_COMPILE_ASSERT

#define SLJIT_COMPILE_ASSERT(x, description) \
	switch(0) { case 0: case ((x) ? 1 : 0): break; }

#endif /* !SLJIT_COMPILE_ASSERT */

#ifndef SLJIT_FALLTHROUGH

#if defined(__GNUC__) && __GNUC__ >= 7
#define SLJIT_FALLTHROUGH __attribute__((fallthrough));
#else
#define SLJIT_FALLTHROUGH
#endif

#endif /* !SLJIT_FALLTHROUGH */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SLJIT_CONFIG_INTERNAL_H_ */
