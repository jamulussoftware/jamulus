/*
Copyright (c) 2003-2004, Mark Borgerding
Lots of modifications by JMV
Copyright (c) 2005-2007, Jean-Marc Valin
Copyright (c) 2008,      Jean-Marc Valin, CSIRO

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef cc6_KISS_FFT_H
#define cc6_KISS_FFT_H

#include <stdlib.h>
#include <math.h>
#include "cc6_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 ATTENTION!
 If you would like a :
 -- a utility that will handle the caching of fft objects
 -- real-only (no imaginary time component ) FFT
 -- a multi-dimensional FFT
 -- a command-line utility to perform ffts
 -- a command-line utility to perform fast-convolution filtering

 Then see kfc.h cc6_kiss_fftr.h kiss_fftnd.h fftutil.c kiss_fastfir.c
  in the tools/ directory.
*/

#ifdef USE_SIMD
# include <xmmintrin.h>
# define cc6_kiss_fft_scalar __m128
#define cc6_KISS_FFT_MALLOC(nbytes) memalign(16,nbytes)
#else	
#define cc6_KISS_FFT_MALLOC cc6_celt_alloc
#endif	


#ifdef FIXED_POINT
#include "cc6_arch.h"
#ifdef DOUBLE_PRECISION
#  define cc6_kiss_fft_scalar cc6_celt_int32_t
#  define cc6_kiss_twiddle_scalar cc6_celt_int32_t
#  define cc6_KF_SUFFIX cc6__celt_double
#else
#  define cc6_kiss_fft_scalar cc6_celt_int16_t
#  define cc6_kiss_twiddle_scalar cc6_celt_int16_t
#  define cc6_KF_SUFFIX _celt_single
#endif
#else
# ifndef cc6_kiss_fft_scalar
/*  default is float */
#   define cc6_kiss_fft_scalar float
#   define cc6_kiss_twiddle_scalar float
#   define cc6_KF_SUFFIX _celt_single
# endif
#endif


/* This adds a suffix to all the kiss_fft functions so we
   can easily link with more than one copy of the fft */
#define cc6_CAT_SUFFIX(a,b) a ## b
#define cc6_SUF(a,b) cc6_CAT_SUFFIX(a, b)

#define cc6_kiss_fft_alloc cc6_SUF(cc6_kiss_fft_alloc,cc6_KF_SUFFIX)
#define cc6_kf_work cc6_SUF(cc6_kf_work,cc6_KF_SUFFIX)
#define cc6_ki_work cc6_SUF(cc6_ki_work,cc6_KF_SUFFIX)
#define cc6_kiss_fft cc6_SUF(cc6_kiss_fft,cc6_KF_SUFFIX)
#define cc6_kiss_ifft cc6_SUF(cc6_kiss_ifft,cc6_KF_SUFFIX)
#define cc6_kiss_fft_stride cc6_SUF(cc6_kiss_fft_stride,cc6_KF_SUFFIX)
#define cc6_kiss_ifft_stride cc6_SUF(cc6_kiss_ifft_stride,cc6_KF_SUFFIX)


typedef struct {
    cc6_kiss_fft_scalar r;
    cc6_kiss_fft_scalar i;
}cc6_kiss_fft_cpx;

typedef struct {
   cc6_kiss_twiddle_scalar r;
   cc6_kiss_twiddle_scalar i;
}cc6_kiss_twiddle_cpx;

typedef struct cc6_kiss_fft_state* cc6_kiss_fft_cfg;

/** 
 *  cc6_kiss_fft_alloc
 *  
 *  Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
 *
 *  typical usage:      cc6_kiss_fft_cfg mycfg=cc6_kiss_fft_alloc(1024,0,NULL,NULL);
 *
 *  The return value from fft_alloc is a cfg buffer used internally
 *  by the fft routine or NULL.
 *
 *  If lenmem is NULL, then cc6_kiss_fft_alloc will allocate a cfg buffer using malloc.
 *  The returned value should be free()d when done to avoid memory leaks.
 *  
 *  The state can be placed in a user supplied buffer 'mem':
 *  If lenmem is not NULL and mem is not NULL and *lenmem is large enough,
 *      then the function places the cfg in mem and the size used in *lenmem
 *      and returns mem.
 *  
 *  If lenmem is not NULL and ( mem is NULL or *lenmem is not large enough),
 *      then the function returns NULL and places the minimum cfg 
 *      buffer size in *lenmem.
 * */

cc6_kiss_fft_cfg cc6_kiss_fft_alloc(int nfft,void * mem,size_t * lenmem);

void cc6_kf_work(cc6_kiss_fft_cpx * Fout,const cc6_kiss_fft_cpx * f,const size_t fstride,
             int in_stride,int * factors,const cc6_kiss_fft_cfg st,int N,int s2,int m2);

/** Internal function. Can be useful when you want to do the bit-reversing yourself */
void cc6_ki_work(cc6_kiss_fft_cpx * Fout, const cc6_kiss_fft_cpx * f, const size_t fstride,
             int in_stride,int * factors,const cc6_kiss_fft_cfg st,int N,int s2,int m2);

/**
 * cc6_kiss_fft(cfg,in_out_buf)
 *
 * Perform an FFT on a complex input buffer.
 * for a forward FFT,
 * fin should be  f[0] , f[1] , ... ,f[nfft-1]
 * fout will be   F[0] , F[1] , ... ,F[nfft-1]
 * Note that each element is complex and can be accessed like
    f[k].r and f[k].i
 * */
void cc6_kiss_fft(cc6_kiss_fft_cfg cfg,const cc6_kiss_fft_cpx *fin,cc6_kiss_fft_cpx *fout);
void cc6_kiss_ifft(cc6_kiss_fft_cfg cfg,const cc6_kiss_fft_cpx *fin,cc6_kiss_fft_cpx *fout);

/**
 A more generic version of the above function. It reads its input from every Nth sample.
 * */
void cc6_kiss_fft_stride(cc6_kiss_fft_cfg cfg,const cc6_kiss_fft_cpx *fin,cc6_kiss_fft_cpx *fout,int fin_stride);
void cc6_kiss_ifft_stride(cc6_kiss_fft_cfg cfg,const cc6_kiss_fft_cpx *fin,cc6_kiss_fft_cpx *fout,int fin_stride);

/** If cc6_kiss_fft_alloc allocated a buffer, it is one contiguous
   buffer and can be simply free()d when no longer needed*/
#define cc6_kiss_fft_free cc6_celt_free


#ifdef __cplusplus
} 
#endif

#endif
