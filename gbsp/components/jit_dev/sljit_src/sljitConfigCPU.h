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
 * SLJIT CPU Configuration for ESP32-P4 (RISC-V 32-bit only)
 * 
 * 精简版：仅保留 RISC-V 32位架构支持
 * 架构由 sljitConfigEsp32P4.h 中的 SLJIT_CONFIG_RISCV_32 定义
 */

#ifndef SLJIT_CONFIG_CPU_H_
#define SLJIT_CONFIG_CPU_H_

/* --------------------------------------------------------------------- */
/*  Architecture Validation                                              */
/* --------------------------------------------------------------------- */

/* 验证 SLJIT_CONFIG_RISCV_32 已由 sljitConfigEsp32P4.h 定义 */
#ifndef SLJIT_CONFIG_RISCV_32
#error "SLJIT_CONFIG_RISCV_32 must be defined in sljitConfigEsp32P4.h"
#endif

/* --------------------------------------------------------------------- */
/*  CPU Family Type                                                      */
/* --------------------------------------------------------------------- */

#define SLJIT_CONFIG_RISCV 1

/* --------------------------------------------------------------------- */
/*  Endianness                                                           */
/* --------------------------------------------------------------------- */

/* RISC-V 默认为小端序 */
#define SLJIT_LITTLE_ENDIAN 1

#endif /* SLJIT_CONFIG_CPU_H_ */
