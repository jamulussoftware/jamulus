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

#ifndef SKIP_CONFIG_H
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif
#endif

#include "cc6_os_support.h"
#include "cc6_mathops.h"
#include "cc6_kiss_fftr.h"
#include "cc6__kiss_fft_guts.h"


cc6_kiss_fftr_cfg cc6_kiss_fftr_alloc(int nfft,void * mem,size_t * lenmem)
{
    int i;
    int twiddle_size;
    cc6_kiss_fftr_cfg st = NULL;
    size_t subsize, memneeded;

    if (nfft & 1) {
        cc6_celt_warning("Real FFT optimization must be even.\n");
        return NULL;
    }
    nfft >>= 1;
    twiddle_size = nfft/2+1;
    cc6_kiss_fft_alloc (nfft, NULL, &subsize);
    memneeded = sizeof(struct cc6_kiss_fftr_state) + subsize + sizeof(cc6_kiss_twiddle_cpx)*twiddle_size;

    if (lenmem == NULL) {
        st = (cc6_kiss_fftr_cfg) cc6_KISS_FFT_MALLOC (memneeded);
    } else {
        if (*lenmem >= memneeded)
            st = (cc6_kiss_fftr_cfg) mem;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;

    st->substate = (cc6_kiss_fft_cfg) (st + 1); /*just beyond cc6_kiss_fftr_state struct */
    st->super_twiddles = (cc6_kiss_twiddle_cpx*) (((char *) st->substate) + subsize);
    cc6_kiss_fft_alloc(nfft, st->substate, &subsize);
#ifndef FIXED_POINT
    st->substate->scale *= .5;
#endif

#if defined (FIXED_POINT) && (!defined(DOUBLE_PRECISION) || defined(MIXED_PRECISION))
    for (i=0;i<twiddle_size;++i) {
       cc6_celt_word32_t phase = i+(nfft>>1);
       kf_cexp2(st->super_twiddles+i, cc6_DIV32(cc6_SHL32(phase,16),nfft));
    }
#else
    for (i=0;i<twiddle_size;++i) {
       const double pi=3.14159265358979323846264338327;
       double phase = pi*(((double)i) /nfft + .5);
       kf_cexp(st->super_twiddles+i, phase );
    }
#endif
    return st;
}

void cc6_kiss_fftr_twiddles(cc6_kiss_fftr_cfg st,cc6_kiss_fft_scalar *freqdata)
{
   /* input buffer timedata is stored row-wise */
   int k,ncfft;
   cc6_kiss_fft_cpx f2k,f1k,tdc,tw;

   ncfft = st->substate->nfft;

    /* The real part of the DC element of the frequency spectrum in st->tmpbuf
   * contains the sum of the even-numbered elements of the input time sequence
   * The imag part is the sum of the odd-numbered elements
   *
   * The sum of tdc.r and tdc.i is the sum of the input time sequence. 
   *      yielding DC of input time sequence
   * The difference of tdc.r - tdc.i is the sum of the input (dot product) [1,-1,1,-1... 
   *      yielding Nyquist bin of input time sequence
    */
 
   tdc.r = freqdata[0];
   tdc.i = freqdata[1];
   cc6_C_FIXDIV(tdc,2);
   cc6_CHECK_OVERFLOW_OP(tdc.r ,+, tdc.i);
   cc6_CHECK_OVERFLOW_OP(tdc.r ,-, tdc.i);
   freqdata[0] = tdc.r + tdc.i;
   freqdata[1] = tdc.r - tdc.i;

   for ( k=1;k <= ncfft/2 ; ++k )
   {
      f2k.r = cc6_SHR32(cc6_SUB32(cc6_EXT32(freqdata[2*k]), cc6_EXT32(freqdata[2*(ncfft-k)])),1);
      f2k.i = cc6_PSHR32(cc6_ADD32(cc6_EXT32(freqdata[2*k+1]), cc6_EXT32(freqdata[2*(ncfft-k)+1])),1);
      
      f1k.r = cc6_SHR32(cc6_ADD32(cc6_EXT32(freqdata[2*k]), cc6_EXT32(freqdata[2*(ncfft-k)])),1);
      f1k.i = cc6_SHR32(cc6_SUB32(cc6_EXT32(freqdata[2*k+1]), cc6_EXT32(freqdata[2*(ncfft-k)+1])),1);
      
      cc6_C_MULC( tw , f2k , st->super_twiddles[k]);
      
      freqdata[2*k] = cc6_HALF_OF(f1k.r + tw.r);
      freqdata[2*k+1] = cc6_HALF_OF(f1k.i + tw.i);
      freqdata[2*(ncfft-k)] = cc6_HALF_OF(f1k.r - tw.r);
      freqdata[2*(ncfft-k)+1] = cc6_HALF_OF(tw.i - f1k.i);

   }
}

void cc6_kiss_fftr(cc6_kiss_fftr_cfg st,const cc6_kiss_fft_scalar *timedata,cc6_kiss_fft_scalar *freqdata)
{
   /*perform the parallel fft of two real signals packed in real,imag*/
   cc6_kiss_fft( st->substate , (const cc6_kiss_fft_cpx*)timedata, (cc6_kiss_fft_cpx *)freqdata );

   cc6_kiss_fftr_twiddles(st,freqdata);
}

void cc6_kiss_fftr_inplace(cc6_kiss_fftr_cfg st, cc6_kiss_fft_scalar *X)
{
   cc6_kf_work((cc6_kiss_fft_cpx*)X, NULL, 1,1, st->substate->factors,st->substate, 1, 1, 1);
   cc6_kiss_fftr_twiddles(st,X);
}

void cc6_kiss_fftri(cc6_kiss_fftr_cfg st,const cc6_kiss_fft_scalar *freqdata,cc6_kiss_fft_scalar *timedata)
{
   /* input buffer timedata is stored row-wise */
   int k, ncfft;

   ncfft = st->substate->nfft;

   timedata[2*st->substate->bitrev[0]] = freqdata[0] + freqdata[1];
   timedata[2*st->substate->bitrev[0]+1] = freqdata[0] - freqdata[1];
   for (k = 1; k <= ncfft / 2; ++k) {
      cc6_kiss_fft_cpx fk, fnkc, fek, fok, tmp;
      int k1, k2;
      k1 = st->substate->bitrev[k];
      k2 = st->substate->bitrev[ncfft-k];
      fk.r = freqdata[2*k];
      fk.i = freqdata[2*k+1];
      fnkc.r = freqdata[2*(ncfft-k)];
      fnkc.i = -freqdata[2*(ncfft-k)+1];

      cc6_C_ADD (fek, fk, fnkc);
      cc6_C_SUB (tmp, fk, fnkc);
      cc6_C_MUL (fok, tmp, st->super_twiddles[k]);
      timedata[2*k1] = fek.r + fok.r;
      timedata[2*k1+1] = fek.i + fok.i;
      timedata[2*k2] = fek.r - fok.r;
      timedata[2*k2+1] = fok.i - fek.i;
   }
   cc6_ki_work((cc6_kiss_fft_cpx*)timedata, NULL, 1,1, st->substate->factors,st->substate, 1, 1, 1);
}
