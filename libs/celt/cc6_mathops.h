/* Copyright (C) 2002-2008 Jean-Marc Valin */
/**
   @file mathops.h
   @brief Various math functions
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

#ifndef cc6_MATHOPS_H
#define cc6_MATHOPS_H

#include "cc6_arch.h"
#include "cc6_entcode.h"
#include "cc6_os_support.h"

#ifndef cc6_OVERRIDE_CELT_ILOG2
/** Integer log in base2. Undefined for zero and negative numbers */
static __inline cc6_celt_int16_t cc6_celt_ilog2(cc6_celt_word32_t x)
{
   cc6_celt_assert2(x>0, "cc6_celt_ilog2() only defined for strictly positive numbers");
   return cc6_EC_ILOG(x)-1;
}
#endif

#ifndef cc6_OVERRIDE_FIND_MAX16
static __inline int cc6_find_max16(cc6_celt_word16_t *x, int len)
{
   cc6_celt_word16_t max_corr=-cc6_VERY_LARGE16;
   int i, id = 0;
   for (i=0;i<len;i++)
   {
      if (x[i] > max_corr)
      {
         id = i;
         max_corr = x[i];
      }
   }
   return id;
}
#endif

#ifndef cc6_OVERRIDE_FIND_MAX32
static __inline int find_max32(cc6_celt_word32_t *x, int len)
{
   cc6_celt_word32_t max_corr=-cc6_VERY_LARGE32;
   int i, id = 0;
   for (i=0;i<len;i++)
   {
      if (x[i] > max_corr)
      {
         id = i;
         max_corr = x[i];
      }
   }
   return id;
}
#endif

#define cc6_FRAC_MUL16(a,b) ((16384+((cc6_celt_int32_t)(cc6_celt_int16_t)(a)*(cc6_celt_int16_t)(b)))>>15)
static __inline cc6_celt_int16_t cc6_bitexact_cos(cc6_celt_int16_t x)
{
   cc6_celt_int32_t tmp;
   cc6_celt_int16_t x2;
   tmp = (4096+((cc6_celt_int32_t)(x)*(x)))>>13;
   if (tmp > 32767)
      tmp = 32767;
   x2 = tmp;
   x2 = (32767-x2) + cc6_FRAC_MUL16(x2, (-7651 + cc6_FRAC_MUL16(x2, (8277 + cc6_FRAC_MUL16(-626, x2)))));
   if (x2 > 32766)
      x2 = 32766;
   return 1+x2;
}


#ifndef FIXED_POINT

#define cc6_celt_sqrt(x) ((float)sqrt(x))
#define cc6_celt_psqrt(x) ((float)sqrt(x))
#define cc6_celt_rsqrt(x) (1.f/cc6_celt_sqrt(x))
#define cc6_celt_acos acos
#define cc6_celt_exp exp
#define cc6_celt_cos_norm(x) (cos((.5f*cc6_M_PI)*(x)))
#define cc6_celt_atan atan
#define cc6_celt_rcp(x) (1.f/(x))
#define cc6_celt_div(a,b) ((a)/(b))

#ifdef FLOAT_APPROX

/* Note: This assumes radix-2 floating point with the exponent at bits 23..30 and an offset of 127
         denorm, +/- inf and NaN are *not* handled */

/** Base-2 log approximation (log2(x)). */
static inline float cc6_celt_log2(float x)
{
   int integer;
   float frac;
   union {
      float f;
      cc6_celt_uint32_t i;
   } in;
   in.f = x;
   integer = (in.i>>23)-127;
   in.i -= integer<<23;
   frac = in.f - 1.5;
   /* -0.41446   0.96093  -0.33981   0.15600 */
   frac = -0.41446 + frac*(0.96093 + frac*(-0.33981 + frac*0.15600));
   return 1+integer+frac;
}

/** Base-2 exponential approximation (2^x). */
static inline float cc6_celt_exp2(float x)
{
   int integer;
   float frac;
   union {
      float f;
      cc6_celt_uint32_t i;
   } res;
   integer = floor(x);
   if (integer < -50)
      return 0;
   frac = x-integer;
   /* K0 = 1, K1 = log(2), K2 = 3-4*log(2), K3 = 3*log(2) - 2 */
   res.f = 1.f + frac * (0.696147f + frac * (0.224411f + 0.079442f*frac));
   res.i = (res.i + (integer<<23)) & 0x7fffffff;
   return res.f;
}

#else
#define cc6_celt_log2(x) (1.442695040888963387*log(x))
#define cc6_celt_exp2(x) (exp(0.6931471805599453094*(x)))
#endif

#endif



#ifdef FIXED_POINT

#include "cc6_os_support.h"

#ifndef cc6_OVERRIDE_CELT_MAXABS16
static inline cc6_celt_word16_t cc6_celt_maxabs16(cc6_celt_word16_t *x, int len)
{
   int i;
   cc6_celt_word16_t maxval = 0;
   for (i=0;i<len;i++)
      maxval = cc6_MAX16(maxval, cc6_ABS16(x[i]));
   return maxval;
}
#endif

/** Integer log in base2. Defined for zero, but not for negative numbers */
static inline cc6_celt_int16_t cc6_celt_zlog2(cc6_celt_word32_t x)
{
   return x <= 0 ? 0 : cc6_celt_ilog2(x);
}

/** Reciprocal sqrt approximation (Q30 input, Q0 output or equivalent) */
static inline cc6_celt_word32_t cc6_celt_rsqrt(cc6_celt_word32_t x)
{
   int k;
   cc6_celt_word16_t n;
   cc6_celt_word32_t rt;
   const cc6_celt_word16_t C[5] = {23126, -11496, 9812, -9097, 4100};
   k = cc6_celt_ilog2(x)>>1;
   x = cc6_VSHR32(x, (k-7)<<1);
   /* Range of n is [-16384,32767] */
   n = x-32768;
   rt = cc6_ADD16(C[0], cc6_MULT16_16_Q15(n, cc6_ADD16(C[1], cc6_MULT16_16_Q15(n, cc6_ADD16(C[2],
              cc6_MULT16_16_Q15(n, cc6_ADD16(C[3], cc6_MULT16_16_Q15(n, (C[4])))))))));
   rt = cc6_VSHR32(rt,k);
   return rt;
}

/** Sqrt approximation (QX input, QX/2 output) */
static inline cc6_celt_word32_t cc6_celt_sqrt(cc6_celt_word32_t x)
{
   int k;
   cc6_celt_word16_t n;
   cc6_celt_word32_t rt;
   const cc6_celt_word16_t C[5] = {23174, 11584, -3011, 1570, -557};
   if (x==0)
      return 0;
   k = (cc6_celt_ilog2(x)>>1)-7;
   x = cc6_VSHR32(x, (k<<1));
   n = x-32768;
   rt = cc6_ADD16(C[0], cc6_MULT16_16_Q15(n, cc6_ADD16(C[1], cc6_MULT16_16_Q15(n, cc6_ADD16(C[2],
              cc6_MULT16_16_Q15(n, cc6_ADD16(C[3], cc6_MULT16_16_Q15(n, (C[4])))))))));
   rt = cc6_VSHR32(rt,7-k);
   return rt;
}

/** Sqrt approximation (QX input, QX/2 output) that assumes that the input is
    strictly positive */
static inline cc6_celt_word32_t cc6_celt_psqrt(cc6_celt_word32_t x)
{
   int k;
   cc6_celt_word16_t n;
   cc6_celt_word32_t rt;
   const cc6_celt_word16_t C[5] = {23174, 11584, -3011, 1570, -557};
   k = (cc6_celt_ilog2(x)>>1)-7;
   x = cc6_VSHR32(x, (k<<1));
   n = x-32768;
   rt = cc6_ADD16(C[0], cc6_MULT16_16_Q15(n, cc6_ADD16(C[1], cc6_MULT16_16_Q15(n, cc6_ADD16(C[2],
              cc6_MULT16_16_Q15(n, cc6_ADD16(C[3], cc6_MULT16_16_Q15(n, (C[4])))))))));
   rt = cc6_VSHR32(rt,7-k);
   return rt;
}

#define cc6_L1 32767
#define cc6_L2 -7651
#define cc6_L3 8277
#define cc6_L4 -626

static inline cc6_celt_word16_t cc6__celt_cos_pi_2(cc6_celt_word16_t x)
{
   cc6_celt_word16_t x2;
   
   x2 = cc6_MULT16_16_P15(x,x);
   return cc6_ADD16(1,cc6_MIN16(32766,cc6_ADD32(cc6_SUB16(cc6_L1,x2), cc6_MULT16_16_P15(x2, cc6_ADD32(cc6_L2, cc6_MULT16_16_P15(x2, cc6_ADD32(cc6_L3, cc6_MULT16_16_P15(cc6_L4, x2
                                                                                ))))))));
}

#undef cc6_L1
#undef cc6_L2
#undef cc6_L3
#undef cc6_L4

static inline cc6_celt_word16_t cc6_celt_cos_norm(cc6_celt_word32_t x)
{
   x = x&0x0001ffff;
   if (x>cc6_SHL32(cc6_EXTEND32(1), 16))
      x = cc6_SUB32(cc6_SHL32(cc6_EXTEND32(1), 17),x);
   if (x&0x00007fff)
   {
      if (x<cc6_SHL32(cc6_EXTEND32(1), 15))
      {
         return cc6__celt_cos_pi_2(cc6_EXTRACT16(x));
      } else {
         return cc6_NEG32(cc6__celt_cos_pi_2(cc6_EXTRACT16(65536-x)));
      }
   } else {
      if (x&0x0000ffff)
         return 0;
      else if (x&0x0001ffff)
         return -32767;
      else
         return 32767;
   }
}

static inline cc6_celt_word16_t cc6_celt_log2(cc6_celt_word32_t x)
{
   int i;
   cc6_celt_word16_t n, frac;
   /*-0.41446   0.96093  -0.33981   0.15600 */
   const cc6_celt_word16_t C[4] = {-6791, 7872, -1392, 319};
   if (x==0)
      return -32767;
   i = cc6_celt_ilog2(x);
   n = cc6_VSHR32(x,i-15)-32768-16384;
   frac = cc6_ADD16(C[0], cc6_MULT16_16_Q14(n, cc6_ADD16(C[1], cc6_MULT16_16_Q14(n, cc6_ADD16(C[2], cc6_MULT16_16_Q14(n, (C[3])))))));
   return cc6_SHL16(i-13,8)+cc6_SHR16(frac,14-8);
}

/*
 K0 = 1
 K1 = log(2)
 K2 = 3-4*log(2)
 K3 = 3*log(2) - 2
*/
#define cc6_D0 16384
#define cc6_D1 11356
#define cc6_D2 3726
#define cc6_D3 1301
/** Base-2 exponential approximation (2^x). (Q11 input, Q16 output) */
static inline cc6_celt_word32_t cc6_celt_exp2(cc6_celt_word16_t x)
{
   int integer;
   cc6_celt_word16_t frac;
   integer = cc6_SHR16(x,11);
   if (integer>14)
      return 0x7f000000;
   else if (integer < -15)
      return 0;
   frac = cc6_SHL16(x-cc6_SHL16(integer,11),3);
   frac = cc6_ADD16(cc6_D0, cc6_MULT16_16_Q14(frac, cc6_ADD16(cc6_D1, cc6_MULT16_16_Q14(frac, cc6_ADD16(cc6_D2 , cc6_MULT16_16_Q14(cc6_D3,frac))))));
   return cc6_VSHR32(cc6_EXTEND32(frac), -integer-2);
}

/** Reciprocal approximation (Q15 input, Q16 output) */
static inline cc6_celt_word32_t cc6_celt_rcp(cc6_celt_word32_t x)
{
   int i;
   cc6_celt_word16_t n, frac;
   const cc6_celt_word16_t C[5] = {21848, -7251, 2403, -934, 327};
   cc6_celt_assert2(x>0, "cc6_celt_rcp() only defined for positive values");
   i = cc6_celt_ilog2(x);
   n = cc6_VSHR32(x,i-16)-cc6_SHL32(cc6_EXTEND32(3),15);
   frac = cc6_ADD16(C[0], cc6_MULT16_16_Q15(n, cc6_ADD16(C[1], cc6_MULT16_16_Q15(n, cc6_ADD16(C[2],
                cc6_MULT16_16_Q15(n, cc6_ADD16(C[3], cc6_MULT16_16_Q15(n, (C[4])))))))));
   return cc6_VSHR32(cc6_EXTEND32(frac),i-16);
}

#define cc6_celt_div(a,b) cc6_MULT32_32_Q31((cc6_celt_word32_t)(a),cc6_celt_rcp(b))


#define cc6_M1 32767
#define cc6_M2 -21
#define cc6_M3 -11943
#define cc6_M4 4936

static inline cc6_celt_word16_t cc6_celt_atan01(cc6_celt_word16_t x)
{
   return cc6_MULT16_16_P15(x, cc6_ADD32(cc6_M1, cc6_MULT16_16_P15(x, cc6_ADD32(cc6_M2, cc6_MULT16_16_P15(x, cc6_ADD32(cc6_M3, cc6_MULT16_16_P15(cc6_M4, x)))))));
}

#undef cc6_M1
#undef cc6_M2
#undef cc6_M3
#undef cc6_M4

static inline cc6_celt_word16_t cc6_celt_atan2p(cc6_celt_word16_t y, cc6_celt_word16_t x)
{
   if (y < x)
   {
      cc6_celt_word32_t arg;
      arg = cc6_celt_div(cc6_SHL32(cc6_EXTEND32(y),15),x);
      if (arg >= 32767)
         arg = 32767;
      return cc6_SHR16(cc6_celt_atan01(cc6_EXTRACT16(arg)),1);
   } else {
      cc6_celt_word32_t arg;
      arg = cc6_celt_div(cc6_SHL32(cc6_EXTEND32(x),15),y);
      if (arg >= 32767)
         arg = 32767;
      return 25736-cc6_SHR16(cc6_celt_atan01(cc6_EXTRACT16(arg)),1);
   }
}

#endif /* FIXED_POINT */


#endif /* cc6_MATHOPS_H */
