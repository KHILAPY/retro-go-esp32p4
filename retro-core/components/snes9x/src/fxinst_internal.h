/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _FXINST_INTERNAL_H_
#define _FXINST_INTERNAL_H_

#include "snes9x.h"
#include "fxinst.h"
#include "fxemu.h"

#define CHECK_LIMITS

#define TEST_S  (GSU.vSign & 0x8000)
#define TEST_Z  (USEX16(GSU.vZero) == 0)
#define TEST_OV (GSU.vOverflow >= 0x8000 || GSU.vOverflow < -0x8000)
#define TEST_CY (GSU.vCarry & 1)

#define BRA_COND(cond) \
    uint8_t v = PIPE; \
    R15++; \
    FETCHPIPE; \
    if (cond) \
        R15 += SEX8(v); \
    else \
        R15++; \
    CLRFLAGS

void fx_stop (void);
void fx_nop (void);
void fx_cache (void);
void fx_loop (void);
void fx_alt1 (void);
void fx_alt2 (void);
void fx_alt3 (void);
void fx_sbk (void);

void fx_bra (void);
void fx_blt (void);
void fx_bge (void);
void fx_bne (void);
void fx_beq (void);
void fx_bpl (void);
void fx_bmi (void);
void fx_bcc (void);
void fx_bcs (void);
void fx_bvc (void);
void fx_bvs (void);
void fx_jmp_r8 (void);
void fx_jmp_r9 (void);
void fx_jmp_r10 (void);
void fx_jmp_r11 (void);
void fx_jmp_r12 (void);
void fx_jmp_r13 (void);
void fx_ljmp_r8 (void);
void fx_ljmp_r9 (void);
void fx_ljmp_r10 (void);
void fx_ljmp_r11 (void);
void fx_ljmp_r12 (void);
void fx_ljmp_r13 (void);

void fx_to_r0 (void);
void fx_to_r1 (void);
void fx_to_r2 (void);
void fx_to_r3 (void);
void fx_to_r4 (void);
void fx_to_r5 (void);
void fx_to_r6 (void);
void fx_to_r7 (void);
void fx_to_r8 (void);
void fx_to_r9 (void);
void fx_to_r10 (void);
void fx_to_r11 (void);
void fx_to_r12 (void);
void fx_to_r13 (void);
void fx_to_r14 (void);
void fx_to_r15 (void);

void fx_with_r0 (void);
void fx_with_r1 (void);
void fx_with_r2 (void);
void fx_with_r3 (void);
void fx_with_r4 (void);
void fx_with_r5 (void);
void fx_with_r6 (void);
void fx_with_r7 (void);
void fx_with_r8 (void);
void fx_with_r9 (void);
void fx_with_r10 (void);
void fx_with_r11 (void);
void fx_with_r12 (void);
void fx_with_r13 (void);
void fx_with_r14 (void);
void fx_with_r15 (void);

void fx_from_r0 (void);
void fx_from_r1 (void);
void fx_from_r2 (void);
void fx_from_r3 (void);
void fx_from_r4 (void);
void fx_from_r5 (void);
void fx_from_r6 (void);
void fx_from_r7 (void);
void fx_from_r8 (void);
void fx_from_r9 (void);
void fx_from_r10 (void);
void fx_from_r11 (void);
void fx_from_r12 (void);
void fx_from_r13 (void);
void fx_from_r14 (void);
void fx_from_r15 (void);

void fx_inc_r0 (void);
void fx_inc_r1 (void);
void fx_inc_r2 (void);
void fx_inc_r3 (void);
void fx_inc_r4 (void);
void fx_inc_r5 (void);
void fx_inc_r6 (void);
void fx_inc_r7 (void);
void fx_inc_r8 (void);
void fx_inc_r9 (void);
void fx_inc_r10 (void);
void fx_inc_r11 (void);
void fx_inc_r12 (void);
void fx_inc_r13 (void);
void fx_inc_r14 (void);

void fx_dec_r0 (void);
void fx_dec_r1 (void);
void fx_dec_r2 (void);
void fx_dec_r3 (void);
void fx_dec_r4 (void);
void fx_dec_r5 (void);
void fx_dec_r6 (void);
void fx_dec_r7 (void);
void fx_dec_r8 (void);
void fx_dec_r9 (void);
void fx_dec_r10 (void);
void fx_dec_r11 (void);
void fx_dec_r12 (void);
void fx_dec_r13 (void);
void fx_dec_r14 (void);

void fx_ibt_r0 (void);
void fx_ibt_r1 (void);
void fx_ibt_r2 (void);
void fx_ibt_r3 (void);
void fx_ibt_r4 (void);
void fx_ibt_r5 (void);
void fx_ibt_r6 (void);
void fx_ibt_r7 (void);
void fx_ibt_r8 (void);
void fx_ibt_r9 (void);
void fx_ibt_r10 (void);
void fx_ibt_r11 (void);
void fx_ibt_r12 (void);
void fx_ibt_r13 (void);
void fx_ibt_r14 (void);
void fx_ibt_r15 (void);

void fx_iwt_r0 (void);
void fx_iwt_r1 (void);
void fx_iwt_r2 (void);
void fx_iwt_r3 (void);
void fx_iwt_r4 (void);
void fx_iwt_r5 (void);
void fx_iwt_r6 (void);
void fx_iwt_r7 (void);
void fx_iwt_r8 (void);
void fx_iwt_r9 (void);
void fx_iwt_r10 (void);
void fx_iwt_r11 (void);
void fx_iwt_r12 (void);
void fx_iwt_r13 (void);
void fx_iwt_r14 (void);
void fx_iwt_r15 (void);

void fx_link_i1 (void);
void fx_link_i2 (void);
void fx_link_i3 (void);
void fx_link_i4 (void);

void fx_add_r0 (void);
void fx_add_r1 (void);
void fx_add_r2 (void);
void fx_add_r3 (void);
void fx_add_r4 (void);
void fx_add_r5 (void);
void fx_add_r6 (void);
void fx_add_r7 (void);
void fx_add_r8 (void);
void fx_add_r9 (void);
void fx_add_r10 (void);
void fx_add_r11 (void);
void fx_add_r12 (void);
void fx_add_r13 (void);
void fx_add_r14 (void);
void fx_add_r15 (void);

void fx_add_i0 (void);
void fx_add_i1 (void);
void fx_add_i2 (void);
void fx_add_i3 (void);
void fx_add_i4 (void);
void fx_add_i5 (void);
void fx_add_i6 (void);
void fx_add_i7 (void);
void fx_add_i8 (void);
void fx_add_i9 (void);
void fx_add_i10 (void);
void fx_add_i11 (void);
void fx_add_i12 (void);
void fx_add_i13 (void);
void fx_add_i14 (void);
void fx_add_i15 (void);

void fx_sub_r0 (void);
void fx_sub_r1 (void);
void fx_sub_r2 (void);
void fx_sub_r3 (void);
void fx_sub_r4 (void);
void fx_sub_r5 (void);
void fx_sub_r6 (void);
void fx_sub_r7 (void);
void fx_sub_r8 (void);
void fx_sub_r9 (void);
void fx_sub_r10 (void);
void fx_sub_r11 (void);
void fx_sub_r12 (void);
void fx_sub_r13 (void);
void fx_sub_r14 (void);
void fx_sub_r15 (void);

void fx_sub_i0 (void);
void fx_sub_i1 (void);
void fx_sub_i2 (void);
void fx_sub_i3 (void);
void fx_sub_i4 (void);
void fx_sub_i5 (void);
void fx_sub_i6 (void);
void fx_sub_i7 (void);
void fx_sub_i8 (void);
void fx_sub_i9 (void);
void fx_sub_i10 (void);
void fx_sub_i11 (void);
void fx_sub_i12 (void);
void fx_sub_i13 (void);
void fx_sub_i14 (void);
void fx_sub_i15 (void);

void fx_cmp_r0 (void);
void fx_cmp_r1 (void);
void fx_cmp_r2 (void);
void fx_cmp_r3 (void);
void fx_cmp_r4 (void);
void fx_cmp_r5 (void);
void fx_cmp_r6 (void);
void fx_cmp_r7 (void);
void fx_cmp_r8 (void);
void fx_cmp_r9 (void);
void fx_cmp_r10 (void);
void fx_cmp_r11 (void);
void fx_cmp_r12 (void);
void fx_cmp_r13 (void);
void fx_cmp_r14 (void);
void fx_cmp_r15 (void);

void fx_adc_r0 (void);
void fx_adc_r1 (void);
void fx_adc_r2 (void);
void fx_adc_r3 (void);
void fx_adc_r4 (void);
void fx_adc_r5 (void);
void fx_adc_r6 (void);
void fx_adc_r7 (void);
void fx_adc_r8 (void);
void fx_adc_r9 (void);
void fx_adc_r10 (void);
void fx_adc_r11 (void);
void fx_adc_r12 (void);
void fx_adc_r13 (void);
void fx_adc_r14 (void);
void fx_adc_r15 (void);

void fx_adc_i0 (void);
void fx_adc_i1 (void);
void fx_adc_i2 (void);
void fx_adc_i3 (void);
void fx_adc_i4 (void);
void fx_adc_i5 (void);
void fx_adc_i6 (void);
void fx_adc_i7 (void);
void fx_adc_i8 (void);
void fx_adc_i9 (void);
void fx_adc_i10 (void);
void fx_adc_i11 (void);
void fx_adc_i12 (void);
void fx_adc_i13 (void);
void fx_adc_i14 (void);
void fx_adc_i15 (void);

void fx_sbc_r0 (void);
void fx_sbc_r1 (void);
void fx_sbc_r2 (void);
void fx_sbc_r3 (void);
void fx_sbc_r4 (void);
void fx_sbc_r5 (void);
void fx_sbc_r6 (void);
void fx_sbc_r7 (void);
void fx_sbc_r8 (void);
void fx_sbc_r9 (void);
void fx_sbc_r10 (void);
void fx_sbc_r11 (void);
void fx_sbc_r12 (void);
void fx_sbc_r13 (void);
void fx_sbc_r14 (void);
void fx_sbc_r15 (void);

void fx_mult_r0 (void);
void fx_mult_r1 (void);
void fx_mult_r2 (void);
void fx_mult_r3 (void);
void fx_mult_r4 (void);
void fx_mult_r5 (void);
void fx_mult_r6 (void);
void fx_mult_r7 (void);
void fx_mult_r8 (void);
void fx_mult_r9 (void);
void fx_mult_r10 (void);
void fx_mult_r11 (void);
void fx_mult_r12 (void);
void fx_mult_r13 (void);
void fx_mult_r14 (void);
void fx_mult_r15 (void);

void fx_umult_r0 (void);
void fx_umult_r1 (void);
void fx_umult_r2 (void);
void fx_umult_r3 (void);
void fx_umult_r4 (void);
void fx_umult_r5 (void);
void fx_umult_r6 (void);
void fx_umult_r7 (void);
void fx_umult_r8 (void);
void fx_umult_r9 (void);
void fx_umult_r10 (void);
void fx_umult_r11 (void);
void fx_umult_r12 (void);
void fx_umult_r13 (void);
void fx_umult_r14 (void);
void fx_umult_r15 (void);

void fx_mult_i0 (void);
void fx_mult_i1 (void);
void fx_mult_i2 (void);
void fx_mult_i3 (void);
void fx_mult_i4 (void);
void fx_mult_i5 (void);
void fx_mult_i6 (void);
void fx_mult_i7 (void);
void fx_mult_i8 (void);
void fx_mult_i9 (void);
void fx_mult_i10 (void);
void fx_mult_i11 (void);
void fx_mult_i12 (void);
void fx_mult_i13 (void);
void fx_mult_i14 (void);
void fx_mult_i15 (void);

void fx_umult_i0 (void);
void fx_umult_i1 (void);
void fx_umult_i2 (void);
void fx_umult_i3 (void);
void fx_umult_i4 (void);
void fx_umult_i5 (void);
void fx_umult_i6 (void);
void fx_umult_i7 (void);
void fx_umult_i8 (void);
void fx_umult_i9 (void);
void fx_umult_i10 (void);
void fx_umult_i11 (void);
void fx_umult_i12 (void);
void fx_umult_i13 (void);
void fx_umult_i14 (void);
void fx_umult_i15 (void);

void fx_fmult (void);
void fx_lmult (void);

void fx_not (void);

void fx_and_r1 (void);
void fx_and_r2 (void);
void fx_and_r3 (void);
void fx_and_r4 (void);
void fx_and_r5 (void);
void fx_and_r6 (void);
void fx_and_r7 (void);
void fx_and_r8 (void);
void fx_and_r9 (void);
void fx_and_r10 (void);
void fx_and_r11 (void);
void fx_and_r12 (void);
void fx_and_r13 (void);
void fx_and_r14 (void);
void fx_and_r15 (void);

void fx_and_i1 (void);
void fx_and_i2 (void);
void fx_and_i3 (void);
void fx_and_i4 (void);
void fx_and_i5 (void);
void fx_and_i6 (void);
void fx_and_i7 (void);
void fx_and_i8 (void);
void fx_and_i9 (void);
void fx_and_i10 (void);
void fx_and_i11 (void);
void fx_and_i12 (void);
void fx_and_i13 (void);
void fx_and_i14 (void);
void fx_and_i15 (void);

void fx_bic_r1 (void);
void fx_bic_r2 (void);
void fx_bic_r3 (void);
void fx_bic_r4 (void);
void fx_bic_r5 (void);
void fx_bic_r6 (void);
void fx_bic_r7 (void);
void fx_bic_r8 (void);
void fx_bic_r9 (void);
void fx_bic_r10 (void);
void fx_bic_r11 (void);
void fx_bic_r12 (void);
void fx_bic_r13 (void);
void fx_bic_r14 (void);
void fx_bic_r15 (void);

void fx_bic_i1 (void);
void fx_bic_i2 (void);
void fx_bic_i3 (void);
void fx_bic_i4 (void);
void fx_bic_i5 (void);
void fx_bic_i6 (void);
void fx_bic_i7 (void);
void fx_bic_i8 (void);
void fx_bic_i9 (void);
void fx_bic_i10 (void);
void fx_bic_i11 (void);
void fx_bic_i12 (void);
void fx_bic_i13 (void);
void fx_bic_i14 (void);
void fx_bic_i15 (void);

void fx_or_r1 (void);
void fx_or_r2 (void);
void fx_or_r3 (void);
void fx_or_r4 (void);
void fx_or_r5 (void);
void fx_or_r6 (void);
void fx_or_r7 (void);
void fx_or_r8 (void);
void fx_or_r9 (void);
void fx_or_r10 (void);
void fx_or_r11 (void);
void fx_or_r12 (void);
void fx_or_r13 (void);
void fx_or_r14 (void);
void fx_or_r15 (void);

void fx_or_i1 (void);
void fx_or_i2 (void);
void fx_or_i3 (void);
void fx_or_i4 (void);
void fx_or_i5 (void);
void fx_or_i6 (void);
void fx_or_i7 (void);
void fx_or_i8 (void);
void fx_or_i9 (void);
void fx_or_i10 (void);
void fx_or_i11 (void);
void fx_or_i12 (void);
void fx_or_i13 (void);
void fx_or_i14 (void);
void fx_or_i15 (void);

void fx_xor_r1 (void);
void fx_xor_r2 (void);
void fx_xor_r3 (void);
void fx_xor_r4 (void);
void fx_xor_r5 (void);
void fx_xor_r6 (void);
void fx_xor_r7 (void);
void fx_xor_r8 (void);
void fx_xor_r9 (void);
void fx_xor_r10 (void);
void fx_xor_r11 (void);
void fx_xor_r12 (void);
void fx_xor_r13 (void);
void fx_xor_r14 (void);
void fx_xor_r15 (void);

void fx_xor_i1 (void);
void fx_xor_i2 (void);
void fx_xor_i3 (void);
void fx_xor_i4 (void);
void fx_xor_i5 (void);
void fx_xor_i6 (void);
void fx_xor_i7 (void);
void fx_xor_i8 (void);
void fx_xor_i9 (void);
void fx_xor_i10 (void);
void fx_xor_i11 (void);
void fx_xor_i12 (void);
void fx_xor_i13 (void);
void fx_xor_i14 (void);
void fx_xor_i15 (void);

void fx_lsr (void);
void fx_rol (void);
void fx_ror (void);
void fx_asr (void);
void fx_div2 (void);
void fx_sex (void);
void fx_swap (void);
void fx_lob (void);
void fx_hib (void);
void fx_merge (void);

void fx_stw_r0 (void);
void fx_stw_r1 (void);
void fx_stw_r2 (void);
void fx_stw_r3 (void);
void fx_stw_r4 (void);
void fx_stw_r5 (void);
void fx_stw_r6 (void);
void fx_stw_r7 (void);
void fx_stw_r8 (void);
void fx_stw_r9 (void);
void fx_stw_r10 (void);
void fx_stw_r11 (void);

void fx_stb_r0 (void);
void fx_stb_r1 (void);
void fx_stb_r2 (void);
void fx_stb_r3 (void);
void fx_stb_r4 (void);
void fx_stb_r5 (void);
void fx_stb_r6 (void);
void fx_stb_r7 (void);
void fx_stb_r8 (void);
void fx_stb_r9 (void);
void fx_stb_r10 (void);
void fx_stb_r11 (void);

void fx_ldw_r0 (void);
void fx_ldw_r1 (void);
void fx_ldw_r2 (void);
void fx_ldw_r3 (void);
void fx_ldw_r4 (void);
void fx_ldw_r5 (void);
void fx_ldw_r6 (void);
void fx_ldw_r7 (void);
void fx_ldw_r8 (void);
void fx_ldw_r9 (void);
void fx_ldw_r10 (void);
void fx_ldw_r11 (void);

void fx_ldb_r0 (void);
void fx_ldb_r1 (void);
void fx_ldb_r2 (void);
void fx_ldb_r3 (void);
void fx_ldb_r4 (void);
void fx_ldb_r5 (void);
void fx_ldb_r6 (void);
void fx_ldb_r7 (void);
void fx_ldb_r8 (void);
void fx_ldb_r9 (void);
void fx_ldb_r10 (void);
void fx_ldb_r11 (void);

void fx_lms_r0 (void);
void fx_lms_r1 (void);
void fx_lms_r2 (void);
void fx_lms_r3 (void);
void fx_lms_r4 (void);
void fx_lms_r5 (void);
void fx_lms_r6 (void);
void fx_lms_r7 (void);
void fx_lms_r8 (void);
void fx_lms_r9 (void);
void fx_lms_r10 (void);
void fx_lms_r11 (void);
void fx_lms_r12 (void);
void fx_lms_r13 (void);
void fx_lms_r14 (void);
void fx_lms_r15 (void);

void fx_sms_r0 (void);
void fx_sms_r1 (void);
void fx_sms_r2 (void);
void fx_sms_r3 (void);
void fx_sms_r4 (void);
void fx_sms_r5 (void);
void fx_sms_r6 (void);
void fx_sms_r7 (void);
void fx_sms_r8 (void);
void fx_sms_r9 (void);
void fx_sms_r10 (void);
void fx_sms_r11 (void);
void fx_sms_r12 (void);
void fx_sms_r13 (void);
void fx_sms_r14 (void);
void fx_sms_r15 (void);

void fx_lm_r0 (void);
void fx_lm_r1 (void);
void fx_lm_r2 (void);
void fx_lm_r3 (void);
void fx_lm_r4 (void);
void fx_lm_r5 (void);
void fx_lm_r6 (void);
void fx_lm_r7 (void);
void fx_lm_r8 (void);
void fx_lm_r9 (void);
void fx_lm_r10 (void);
void fx_lm_r11 (void);
void fx_lm_r12 (void);
void fx_lm_r13 (void);
void fx_lm_r14 (void);
void fx_lm_r15 (void);

void fx_sm_r0 (void);
void fx_sm_r1 (void);
void fx_sm_r2 (void);
void fx_sm_r3 (void);
void fx_sm_r4 (void);
void fx_sm_r5 (void);
void fx_sm_r6 (void);
void fx_sm_r7 (void);
void fx_sm_r8 (void);
void fx_sm_r9 (void);
void fx_sm_r10 (void);
void fx_sm_r11 (void);
void fx_sm_r12 (void);
void fx_sm_r13 (void);
void fx_sm_r14 (void);
void fx_sm_r15 (void);

void fx_ramb (void);
void fx_romb (void);

void fx_color (void);
void fx_cmode (void);
void fx_getc (void);
void fx_getb (void);
void fx_getbh (void);
void fx_getbl (void);
void fx_getbs (void);

void fx_plot_2bit (void);
void fx_plot_4bit (void);
void fx_plot_8bit (void);
void fx_plot_obj (void);

void fx_rpix_2bit (void);
void fx_rpix_4bit (void);
void fx_rpix_8bit (void);
void fx_rpix_obj (void);

extern void (*fx_PlotTable[]) (void);

#endif
