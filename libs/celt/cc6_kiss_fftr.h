/*
Original version:
Copyright (c) 2003-2004, Mark Borgerding
Followed by heavy modifications:
Copyright (c) 2007-2008, Jean-Marc Valin


All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef cc6_KISS_FTR_H
#define cc6_KISS_FTR_H

#include "cc6_kiss_fft.h"
#ifdef __cplusplus
extern "C" {
#endif

#define cc6_kiss_fftr_alloc cc6_SUF(cc6_kiss_fftr_alloc,cc6_KF_SUFFIX)
#define cc6_kiss_fftr_inplace cc6_SUF(cc6_kiss_fftr_inplace,cc6_KF_SUFFIX)
#define cc6_kiss_fftr_alloc cc6_SUF(cc6_kiss_fftr_alloc,cc6_KF_SUFFIX)
#define cc6_kiss_fftr_twiddles cc6_SUF(cc6_kiss_fftr_twiddles,cc6_KF_SUFFIX)
#define cc6_kiss_fftr cc6_SUF(cc6_kiss_fftr,cc6_KF_SUFFIX)
#define cc6_kiss_fftri cc6_SUF(cc6_kiss_fftri,cc6_KF_SUFFIX)

/* 
 
 Real optimized version can save about 45% cpu time vs. complex fft of a real seq.

 
 
 */

struct cc6_kiss_fftr_state{
      cc6_kiss_fft_cfg substate;
      cc6_kiss_twiddle_cpx * super_twiddles;
#ifdef USE_SIMD    
      long pad;
#endif    
   };

typedef struct cc6_kiss_fftr_state *cc6_kiss_fftr_cfg;


cc6_kiss_fftr_cfg cc6_kiss_fftr_alloc(int nfft,void * mem, size_t * lenmem);
/*
 nfft must be even

 If you don't care to allocate space, use mem = lenmem = NULL 
*/


/*
 input timedata has nfft scalar points
 output freqdata has nfft/2+1 complex points, packed into nfft scalar points
*/
void cc6_kiss_fftr_twiddles(cc6_kiss_fftr_cfg st,cc6_kiss_fft_scalar *freqdata);

void cc6_kiss_fftr(cc6_kiss_fftr_cfg st,const cc6_kiss_fft_scalar *timedata,cc6_kiss_fft_scalar *freqdata);
void cc6_kiss_fftr_inplace(cc6_kiss_fftr_cfg st, cc6_kiss_fft_scalar *X);

void cc6_kiss_fftri(cc6_kiss_fftr_cfg st,const cc6_kiss_fft_scalar *freqdata, cc6_kiss_fft_scalar *timedata);

/*
 input freqdata has  nfft/2+1 complex points, packed into nfft scalar points
 output timedata has nfft scalar points
*/

#define cc6_kiss_fftr_free speex_free

#ifdef __cplusplus
}
#endif
#endif
