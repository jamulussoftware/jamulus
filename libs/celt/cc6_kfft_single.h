/* (C) 2008 Jean-Marc Valin, CSIRO
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

#ifndef cc6_KFFT_SINGLE_H
#define cc6_KFFT_SINGLE_H

#ifdef ENABLE_TI_DSPLIB

#include "dsplib.h"

#define cc6_real16_fft_alloc(length) NULL
#define cc6_real16_fft_free(state)
#define cc6_BITREV(state, i) (i)

#define cc6_real16_fft_inplace(state, X, nx)\
    (\
      cfft_SCALE(X,nx/2),\
      cbrev(X,X,nx/2),\
      unpack(X,nx)\
    )

#define cc6_real16_ifft(state, X, Y, nx) \
    (\
      unpacki(X, nx),\
      cifft_NOSCALE(X,nx/2),\
      cbrev(X,Y,nx/2)\
    )


#else /* ENABLE_TI_DSPLIB */

#ifdef FIXED_POINT

#ifdef DOUBLE_PRECISION
#undef DOUBLE_PRECISION
#endif

#ifdef MIXED_PRECISION
#undef MIXED_PRECISION
#endif

#endif /* FIXED_POINT */

#include "cc6_kiss_fft.h"
#include "cc6_kiss_fftr.h"
#include "cc6__kiss_fft_guts.h"

#define cc6_real16_fft_alloc(length) cc6_kiss_fftr_alloc_celt_single(length, 0, 0);
#define cc6_real16_fft_free(state) cc6_kiss_fft_free(state)
#define cc6_real16_fft_inplace(state, X, nx) cc6_kiss_fftr_inplace(state,X)
#define cc6_BITREV(state, i) ((state)->substate->bitrev[i])
#define cc6_real16_ifft(state, X, Y, nx) cc6_kiss_fftri(state,X, Y)

#endif /* !ENABLE_TI_DSPLIB */

#endif /* cc6_KFFT_SINGLE_H */
