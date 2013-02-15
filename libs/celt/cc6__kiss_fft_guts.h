/*
Copyright (c) 2003-2004, Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef cc6_KISS_FFT_GUTS_H
#define cc6_KISS_FFT_GUTS_H

#define cc6_MIN(a,b) ((a)<(b) ? (a):(b))
#define cc6_MAX(a,b) ((a)>(b) ? (a):(b))

/* cc6_kiss_fft.h
   defines cc6_kiss_fft_scalar as either short or a float type
   and defines
   typedef struct { cc6_kiss_fft_scalar r; cc6_kiss_fft_scalar i; }cc6_kiss_fft_cpx; */
#include "cc6_kiss_fft.h"

#define cc6_MAXFACTORS 32
/* e.g. an fft of length 128 has 4 factors 
 as far as kissfft is concerned
 4*4*4*2
 */

struct cc6_kiss_fft_state{
    int nfft;
#ifndef FIXED_POINT
    cc6_kiss_fft_scalar scale;
#endif
    int factors[2*cc6_MAXFACTORS];
    int *bitrev;
    cc6_kiss_twiddle_cpx twiddles[1];
};

/*
  Explanation of macros dealing with complex math:

   cc6_C_MUL(m,a,b)         : m = a*b
   cc6_C_FIXDIV( c , div )  : if a fixed point impl., c /= div. noop otherwise
   cc6_C_SUB( res, a,b)     : res = a - b
   cc6_C_SUBFROM( res , a)  : res -= a
   cc6_C_ADDTO( res , a)    : res += a
 * */
#ifdef FIXED_POINT
#include "cc6_arch.h"

#ifdef DOUBLE_PRECISION

# define cc6_FRACBITS 31
# define cc6_SAMPPROD cc6_celt_int64_t
#define cc6_SAMP_MAX 2147483647
#ifdef MIXED_PRECISION
#define cc6_TWID_MAX 32767
#define cc6_TRIG_UPSCALE 1
#else
#define cc6_TRIG_UPSCALE 65536
#define cc6_TWID_MAX 2147483647
#endif
#define cc6_EXT32(a) (a)

#else /* DOUBLE_PRECISION */

# define cc6_FRACBITS 15
# define cc6_SAMPPROD cc6_celt_int32_t
#define cc6_SAMP_MAX 32767
#define cc6_TRIG_UPSCALE 1
#define cc6_EXT32(a) cc6_EXTEND32(a)

#endif /* !DOUBLE_PRECISION */

#define cc6_SAMP_MIN -cc6_SAMP_MAX

#if defined(CHECK_OVERFLOW)
#  define cc6_CHECK_OVERFLOW_OP(a,op,b)  \
    if ( (cc6_SAMPPROD)(a) op (cc6_SAMPPROD)(b) > cc6_SAMP_MAX || (cc6_SAMPPROD)(a) op (cc6_SAMPPROD)(b) < cc6_SAMP_MIN ) { \
        fprintf(stderr,"WARNING:overflow @ " __FILE__ "(%d): (%d " #op" %d) = %ld\n",__LINE__,(a),(b),(cc6_SAMPPROD)(a) op (cc6_SAMPPROD)(b) );  }
#endif

#   define cc6_smul(a,b) ( (cc6_SAMPPROD)(a)*(b) )
#   define cc6_sround( x )  (cc6_kiss_fft_scalar)( ( (x) + ((cc6_SAMPPROD)1<<(cc6_FRACBITS-1)) ) >> cc6_FRACBITS )

#ifdef MIXED_PRECISION

#   define cc6_S_MUL(a,b) cc6_MULT16_32_Q15(b, a)

#   define cc6_C_MUL(m,a,b) \
      do{ (m).r = cc6_SUB32(cc6_S_MUL((a).r,(b).r) , cc6_S_MUL((a).i,(b).i)); \
          (m).i = cc6_ADD32(cc6_S_MUL((a).r,(b).i) , cc6_S_MUL((a).i,(b).r)); }while(0)

#   define cc6_C_MULC(m,a,b) \
      do{ (m).r = cc6_ADD32(cc6_S_MUL((a).r,(b).r) , cc6_S_MUL((a).i,(b).i)); \
          (m).i = cc6_SUB32(cc6_S_MUL((a).i,(b).r) , cc6_S_MUL((a).r,(b).i)); }while(0)

#   define cc6_C_MUL4(m,a,b) \
      do{ (m).r = cc6_SHR(cc6_SUB32(cc6_S_MUL((a).r,(b).r) , cc6_S_MUL((a).i,(b).i)),2); \
          (m).i = cc6_SHR(cc6_ADD32(cc6_S_MUL((a).r,(b).i) , cc6_S_MUL((a).i,(b).r)),2); }while(0)

#   define cc6_C_MULBYSCALAR( c, s ) \
      do{ (c).r =  cc6_S_MUL( (c).r , s ) ;\
          (c).i =  cc6_S_MUL( (c).i , s ) ; }while(0)

#   define cc6_DIVSCALAR(x,k) \
        (x) = cc6_S_MUL(  x, (cc6_TWID_MAX-((k)>>1))/(k)+1 )

#   define cc6_C_FIXDIV(c,div) \
        do {    cc6_DIVSCALAR( (c).r , div);  \
                cc6_DIVSCALAR( (c).i  , div); }while (0)

#define  cc6_C_ADD( res, a,b)\
    do {(res).r=cc6_ADD32((a).r,(b).r);  (res).i=cc6_ADD32((a).i,(b).i); \
    }while(0)
#define  cc6_C_SUB( res, a,b)\
    do {(res).r=cc6_SUB32((a).r,(b).r);  (res).i=cc6_SUB32((a).i,(b).i); \
    }while(0)
#define cc6_C_ADDTO( res , a)\
    do {(res).r = cc6_ADD32((res).r, (a).r);  (res).i = cc6_ADD32((res).i,(a).i);\
    }while(0)

#define cc6_C_SUBFROM( res , a)\
    do {(res).r = cc6_ADD32((res).r,(a).r);  (res).i = cc6_SUB32((res).i,(a).i); \
    }while(0)

#else /* MIXED_PRECISION */
#   define cc6_sround4( x )  (cc6_kiss_fft_scalar)( ( (x) + ((cc6_SAMPPROD)1<<(cc6_FRACBITS-1)) ) >> (cc6_FRACBITS+2) )

#   define cc6_S_MUL(a,b) cc6_sround( cc6_smul(a,b) )

#   define cc6_C_MUL(m,a,b) \
      do{ (m).r = cc6_sround( cc6_smul((a).r,(b).r) - cc6_smul((a).i,(b).i) ); \
          (m).i = cc6_sround( cc6_smul((a).r,(b).i) + cc6_smul((a).i,(b).r) ); }while(0)
#   define cc6_C_MULC(m,a,b) \
      do{ (m).r = cc6_sround( cc6_smul((a).r,(b).r) + cc6_smul((a).i,(b).i) ); \
          (m).i = cc6_sround( cc6_smul((a).i,(b).r) - cc6_smul((a).r,(b).i) ); }while(0)

#   define cc6_C_MUL4(m,a,b) \
               do{ (m).r = cc6_sround4( cc6_smul((a).r,(b).r) - cc6_smul((a).i,(b).i) ); \
               (m).i = cc6_sround4( cc6_smul((a).r,(b).i) + cc6_smul((a).i,(b).r) ); }while(0)

#   define cc6_C_MULBYSCALAR( c, s ) \
               do{ (c).r =  cc6_sround( cc6_smul( (c).r , s ) ) ;\
               (c).i =  cc6_sround( cc6_smul( (c).i , s ) ) ; }while(0)

#   define cc6_DIVSCALAR(x,k) \
    (x) = cc6_sround( cc6_smul(  x, cc6_SAMP_MAX/k ) )

#   define cc6_C_FIXDIV(c,div) \
    do {    cc6_DIVSCALAR( (c).r , div);  \
        cc6_DIVSCALAR( (c).i  , div); }while (0)

#endif /* !MIXED_PRECISION */



#else  /* not FIXED_POINT*/

#define cc6_EXT32(a) (a)

#   define cc6_S_MUL(a,b) ( (a)*(b) )
#define cc6_C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#define cc6_C_MULC(m,a,b) \
    do{ (m).r = (a).r*(b).r + (a).i*(b).i;\
        (m).i = (a).i*(b).r - (a).r*(b).i; }while(0)

#define cc6_C_MUL4(m,a,b) cc6_C_MUL(m,a,b)

#   define cc6_C_FIXDIV(c,div) /* NOOP */
#   define cc6_C_MULBYSCALAR( c, s ) \
    do{ (c).r *= (s);\
        (c).i *= (s); }while(0)
#endif



#ifndef cc6_CHECK_OVERFLOW_OP
#  define cc6_CHECK_OVERFLOW_OP(a,op,b) /* noop */
#endif

#ifndef cc6_C_ADD
#define  cc6_C_ADD( res, a,b)\
    do { \
        cc6_CHECK_OVERFLOW_OP((a).r,+,(b).r)\
        cc6_CHECK_OVERFLOW_OP((a).i,+,(b).i)\
	    (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  cc6_C_SUB( res, a,b)\
    do { \
        cc6_CHECK_OVERFLOW_OP((a).r,-,(b).r)\
        cc6_CHECK_OVERFLOW_OP((a).i,-,(b).i)\
	    (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)
#define cc6_C_ADDTO( res , a)\
    do { \
        cc6_CHECK_OVERFLOW_OP((res).r,+,(a).r)\
        cc6_CHECK_OVERFLOW_OP((res).i,+,(a).i)\
	    (res).r += (a).r;  (res).i += (a).i;\
    }while(0)

#define cc6_C_SUBFROM( res , a)\
    do {\
        cc6_CHECK_OVERFLOW_OP((res).r,-,(a).r)\
        cc6_CHECK_OVERFLOW_OP((res).i,-,(a).i)\
	    (res).r -= (a).r;  (res).i -= (a).i; \
    }while(0)
#endif /* cc6_C_ADD defined */

#ifdef FIXED_POINT
/*#  define cc6_KISS_FFT_COS(phase)  cc6_TRIG_UPSCALE*floor(cc6_MIN(32767,cc6_MAX(-32767,.5+32768 * cos (phase))))
#  define cc6_KISS_FFT_SIN(phase)  cc6_TRIG_UPSCALE*floor(cc6_MIN(32767,cc6_MAX(-32767,.5+32768 * sin (phase))))*/
#  define cc6_KISS_FFT_COS(phase)  floor(.5+cc6_TWID_MAX*cos (phase))
#  define cc6_KISS_FFT_SIN(phase)  floor(.5+cc6_TWID_MAX*sin (phase))
#  define cc6_HALF_OF(x) ((x)>>1)
#elif defined(USE_SIMD)
#  define cc6_KISS_FFT_COS(phase) _mm_set1_ps( cos(phase) )
#  define cc6_KISS_FFT_SIN(phase) _mm_set1_ps( sin(phase) )
#  define cc6_HALF_OF(x) ((x)*_mm_set1_ps(.5))
#else
#  define cc6_KISS_FFT_COS(phase) (cc6_kiss_fft_scalar) cos(phase)
#  define cc6_KISS_FFT_SIN(phase) (cc6_kiss_fft_scalar) sin(phase)
#  define cc6_HALF_OF(x) ((x)*.5)
#endif

#define  kf_cexp(x,phase) \
	do{ \
        (x)->r = cc6_KISS_FFT_COS(phase);\
        (x)->i = cc6_KISS_FFT_SIN(phase);\
	}while(0)
   
#define  kf_cexp2(x,phase) \
   do{ \
      (x)->r = cc6_TRIG_UPSCALE*cc6_celt_cos_norm((phase));\
      (x)->i = cc6_TRIG_UPSCALE*cc6_celt_cos_norm((phase)-32768);\
}while(0)


#endif /* cc6_KISS_FFT_GUTS_H */
