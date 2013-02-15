/* Copyright (C) 2003-2008 Jean-Marc Valin */
/**
   @file arch.h
   @brief Various architecture definitions for CELT
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef cc6_ARCH_H
#define cc6_ARCH_H

#include "cc6_celt_types.h"

#define cc6_CELT_SIG_SCALE 32768.

#define cc6_celt_fatal(str) cc6__celt_fatal(str, __FILE__, __LINE__);
#ifdef ENABLE_ASSERTIONS
#define cc6_celt_assert(cond) {if (!(cond)) {cc6_celt_fatal("assertion failed: " #cond);}}
#define cc6_celt_assert2(cond, message) {if (!(cond)) {cc6_celt_fatal("assertion failed: " #cond "\n" message);}}
#else
#define cc6_celt_assert(cond)
#define cc6_celt_assert2(cond, message)
#endif

#define cc6_IMUL32(a,b) ((a)*(b))
#define cc6_UMUL32(a,b) ((cc6_celt_int32_t)(a)*(cc6_celt_int32_t)(b))
#define cc6_UMUL16_16(a,b) ((cc6_celt_int32_t)(a)*(cc6_celt_int32_t)(b))

#define cc6_ABS(x) ((x) < 0 ? (-(x)) : (x))      /**< Absolute integer value. */
#define cc6_ABS16(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 16-bit value.  */
#define cc6_MIN16(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum 16-bit value.   */
#define cc6_MAX16(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 16-bit value.   */
#define cc6_ABS32(x) ((x) < 0 ? (-(x)) : (x))    /**< Absolute 32-bit value.  */
#define cc6_MIN32(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum 32-bit value.   */
#define cc6_MAX32(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum 32-bit value.   */
#define cc6_IMIN(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum int value.   */
#define cc6_IMAX(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum int value.   */
#define cc6_UADD32(a,b) ((a)+(b))
#define cc6_USUB32(a,b) ((a)-(b))

#define cc6_PRINT_MIPS(file)

#ifdef FIXED_POINT

typedef cc6_celt_int16_t cc6_celt_word16_t;
typedef cc6_celt_int32_t cc6_celt_word32_t;

typedef cc6_celt_word32_t cc6_celt_sig_t;
typedef cc6_celt_word16_t cc6_celt_norm_t;
typedef cc6_celt_word32_t cc6_celt_ener_t;
typedef cc6_celt_word16_t cc6_celt_pgain_t;
typedef cc6_celt_word32_t cc6_celt_mask_t;

#define cc6_Q15ONE 32767
#define cc6_Q30ONE 1073741823

#define cc6_SIG_SHIFT 12

#define cc6_NORM_SCALING 16384
#define cc6_NORM_SCALING_1 (1.f/16384.f)
#define cc6_NORM_SHIFT 14

#define cc6_ENER_SCALING 16384.f
#define cc6_ENER_SCALING_1 (1.f/16384.f)
#define cc6_ENER_SHIFT 14

#define cc6_PGAIN_SCALING 32768.f
#define cc6_PGAIN_SCALING_1 (1.f/32768.f)
#define cc6_PGAIN_SHIFT 15

#define cc6_DB_SCALING 256.f
#define cc6_DB_SCALING_1 (1.f/256.f)

#define cc6_EPSILON 1
#define cc6_VERY_SMALL 0
#define cc6_VERY_LARGE32 ((cc6_celt_word32_t)2147483647)
#define cc6_VERY_LARGE16 ((cc6_celt_word16_t)32767)
#define cc6_Q15_ONE ((cc6_celt_word16_t)32767)
#define cc6_Q15_ONE_1 (1.f/32768.f)

#define cc6_SCALEIN(a)	(a)
#define cc6_SCALEOUT(a)	(a)

#ifdef FIXED_DEBUG
#include "fixed_debug.h"
#else

#include "cc6_fixed_generic.h"

#ifdef ARM5E_ASM
#include "fixed_arm5e.h"
#elif defined (ARM4_ASM)
#include "fixed_arm4.h"
#elif defined (BFIN_ASM)
#include "fixed_bfin.h"
#elif defined (TI_C5X_ASM)
#include "cc6_fixed_c5x.h"
#elif defined (TI_C6X_ASM)
#include "cc6_fixed_c6x.h"
#endif

#endif


#else /* FIXED_POINT */

typedef float cc6_celt_word16_t;
typedef float cc6_celt_word32_t;

typedef float cc6_celt_sig_t;
typedef float cc6_celt_norm_t;
typedef float cc6_celt_ener_t;
typedef float cc6_celt_pgain_t;
typedef float cc6_celt_mask_t;

#define cc6_Q15ONE 1.0f
#define cc6_Q30ONE 1.0f

#define cc6_NORM_SCALING 1.f
#define cc6_NORM_SCALING_1 1.f
#define cc6_ENER_SCALING 1.f
#define cc6_ENER_SCALING_1 1.f
#define cc6_PGAIN_SCALING 1.f
#define cc6_PGAIN_SCALING_1 1.f

#define cc6_DB_SCALING 1.f
#define cc6_DB_SCALING_1 1.f

#define cc6_EPSILON 1e-15f
#define cc6_VERY_SMALL 1e-15f
#define cc6_VERY_LARGE32 1e15f
#define cc6_VERY_LARGE16 1e15f
#define cc6_Q15_ONE ((cc6_celt_word16_t)1.f)
#define cc6_Q15_ONE_1 ((cc6_celt_word16_t)1.f)

#define cc6_QCONST16(x,bits) (x)
#define cc6_QCONST32(x,bits) (x)

#define cc6_NEG16(x) (-(x))
#define cc6_NEG32(x) (-(x))
#define cc6_EXTRACT16(x) (x)
#define cc6_EXTEND32(x) (x)
#define cc6_SHR16(a,shift) (a)
#define cc6_SHL16(a,shift) (a)
#define cc6_SHR32(a,shift) (a)
#define cc6_SHL32(a,shift) (a)
#define cc6_PSHR16(a,shift) (a)
#define cc6_PSHR32(a,shift) (a)
#define cc6_VSHR32(a,shift) (a)
#define cc6_SATURATE16(x,a) (x)
#define cc6_SATURATE32(x,a) (x)

#define cc6_PSHR(a,shift)   (a)
#define cc6_SHR(a,shift)    (a)
#define cc6_SHL(a,shift)    (a)
#define cc6_SATURATE(x,a)   (x)

#define cc6_ROUND16(a,shift)  (a)
#define cc6_HALF32(x)       (.5f*(x))

#define cc6_ADD16(a,b) ((a)+(b))
#define cc6_SUB16(a,b) ((a)-(b))
#define cc6_ADD32(a,b) ((a)+(b))
#define cc6_SUB32(a,b) ((a)-(b))
#define cc6_MULT16_16_16(a,b)     ((a)*(b))
#define cc6_MULT16_16(a,b)     ((cc6_celt_word32_t)(a)*(cc6_celt_word32_t)(b))
#define cc6_MAC16_16(c,a,b)     ((c)+(cc6_celt_word32_t)(a)*(cc6_celt_word32_t)(b))

#define cc6_MULT16_32_Q11(a,b)     ((a)*(b))
#define cc6_MULT16_32_Q13(a,b)     ((a)*(b))
#define cc6_MULT16_32_Q14(a,b)     ((a)*(b))
#define cc6_MULT16_32_Q15(a,b)     ((a)*(b))
#define cc6_MULT16_32_Q16(a,b)     ((a)*(b))
#define cc6_MULT16_32_P15(a,b)     ((a)*(b))

#define cc6_MULT32_32_Q31(a,b)     ((a)*(b))

#define cc6_MAC16_32_Q11(c,a,b)     ((c)+(a)*(b))
#define cc6_MAC16_32_Q15(c,a,b)     ((c)+(a)*(b))

#define cc6_MAC16_16_Q11(c,a,b)     ((c)+(a)*(b))
#define cc6_MAC16_16_Q13(c,a,b)     ((c)+(a)*(b))
#define cc6_MAC16_16_P13(c,a,b)     ((c)+(a)*(b))
#define cc6_MULT16_16_Q11_32(a,b)     ((a)*(b))
#define cc6_MULT16_16_Q13(a,b)     ((a)*(b))
#define cc6_MULT16_16_Q14(a,b)     ((a)*(b))
#define cc6_MULT16_16_Q15(a,b)     ((a)*(b))
#define cc6_MULT16_16_P15(a,b)     ((a)*(b))
#define cc6_MULT16_16_P13(a,b)     ((a)*(b))
#define cc6_MULT16_16_P14(a,b)     ((a)*(b))

#define cc6_DIV32_16(a,b)     (((cc6_celt_word32_t)(a))/(cc6_celt_word16_t)(b))
#define cc6_PDIV32_16(a,b)     (((cc6_celt_word32_t)(a))/(cc6_celt_word16_t)(b))
#define cc6_DIV32(a,b)     (((cc6_celt_word32_t)(a))/(cc6_celt_word32_t)(b))
#define cc6_PDIV32(a,b)     (((cc6_celt_word32_t)(a))/(cc6_celt_word32_t)(b))

#define cc6_SCALEIN(a)	((a)*cc6_CELT_SIG_SCALE)
#define cc6_SCALEOUT(a)	((a)*(1/cc6_CELT_SIG_SCALE))

#endif /* !FIXED_POINT */


#if defined (CONFIG_TI_C54X) || defined (CONFIG_TI_C55X)

/* 2 on TI C5x DSP */
#define cc6_BYTES_PER_CHAR 2
#define cc6_BITS_PER_CHAR 16
#define cc6_LOG2_BITS_PER_CHAR 4

#else /* CONFIG_TI_C54X */

#define cc6_BYTES_PER_CHAR 1
#define cc6_BITS_PER_CHAR 8
#define cc6_LOG2_BITS_PER_CHAR 3

#endif /* !CONFIG_TI_C54X */

#ifndef cc6_GLOBAL_STACK_SIZE
#ifdef FIXED_POINT
#define cc6_GLOBAL_STACK_SIZE 100000
#else
#define cc6_GLOBAL_STACK_SIZE 100000
#endif
#endif 

#endif /* cc6_ARCH_H */
