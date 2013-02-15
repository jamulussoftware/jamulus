/* (C) 2007 Jean-Marc Valin, CSIRO
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cc6_psy.h"
#include <math.h>
#include "cc6_os_support.h"
#include "cc6_arch.h"
#include "cc6_stack_alloc.h"
#include "cc6_mathops.h"

/* The Vorbis freq<->Bark mapping */
#define cc6_toBARK(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))
#define cc6_fromBARK(z) (102.f*(z)-2.f*pow(z,2.f)+.4f*pow(z,3.f)+pow(1.46f,z)-1.f)

#ifndef STATIC_MODES
/* Psychoacoustic spreading function. The idea here is compute a first order 
   recursive filter. The filter coefficient is frequency dependent and 
   chosen such that we have a -10dB/Bark slope on the right side and a -25dB/Bark
   slope on the left side. */
void cc6_psydecay_init(struct cc6_PsyDecay *decay, int len, cc6_celt_int32_t Fs)
{
   int i;
   cc6_celt_word16_t *decayR = (cc6_celt_word16_t*)cc6_celt_alloc(sizeof(cc6_celt_word16_t)*len);
   decay->decayR = decayR;
   if (decayR==NULL)
     return;
   for (i=0;i<len;i++)
   {
      float f;
      float deriv;
      /* Real frequency (in Hz) */
      f = Fs*i*(1/(2.f*len));
      /* This is the derivative of the Vorbis freq->Bark function (see above) */
      deriv = (8.288e-8 * f)/(3.4225e-16 *f*f*f*f + 1) +  .009694/(5.476e-7 *f*f + 1) + 1e-4;
      /* Back to FFT bin units */
      deriv *= Fs*(1/(2.f*len));
      /* decay corresponding to -10dB/Bark */
      decayR[i] = cc6_Q15ONE*pow(.1f, deriv);
      /* decay corresponding to -25dB/Bark */
      /*decay->decayL[i] = cc6_Q15ONE*pow(0.0031623f, deriv);*/
      /*printf ("%f %f\n", decayL[i], decayR[i]);*/
   }
}

void cc6_psydecay_clear(struct cc6_PsyDecay *decay)
{
   cc6_celt_free((cc6_celt_word16_t *)decay->decayR);
   /*cc6_celt_free(decay->decayL);*/
}
#endif

static void cc6_spreading_func(const struct cc6_PsyDecay *d, cc6_celt_word32_t * __restrict psd, int len)
{
   int i;
   cc6_celt_word32_t mem;
   /* Compute right slope (-10 dB/Bark) */
   mem=psd[0];
   for (i=0;i<len;i++)
   {
      /* psd = (1-decay)*psd + decay*mem */
      psd[i] = cc6_EPSILON + psd[i] + cc6_MULT16_32_Q15(d->decayR[i],mem-psd[i]);
      mem = psd[i];
   }
   /* Compute left slope (-25 dB/Bark) */
   mem=psd[len-1];
   for (i=len-1;i>=0;i--)
   {
      /* Left side has around twice the slope as the right side, so we just
         square the coef instead of storing two sets of decay coefs */
      cc6_celt_word16_t decayL = cc6_MULT16_16_Q15(d->decayR[i], d->decayR[i]);
      /* psd = (1-decay)*psd + decay*mem */
      psd[i] = cc6_EPSILON + psd[i] + cc6_MULT16_32_Q15(decayL,mem-psd[i]);
      mem = psd[i];
   }
#if 0 /* Prints signal and mask energy per critical band */
   for (i=0;i<25;i++)
   {
      int start,end;
      int j;
      cc6_celt_word32_t Esig=0, Emask=0;
      start = (int)floor(cc6_fromBARK((float)i)*(2*len)/Fs);
      if (start<0)
         start = 0;
      end = (int)ceil(cc6_fromBARK((float)(i+1))*(2*len)/Fs);
      if (end<=start)
         end = start+1;
      if (end>len-1)
         end = len-1;
      for (j=start;j<end;j++)
      {
         Esig += psd[j];
         Emask += mask[j];
      }
      printf ("%f %f ", Esig, Emask);
   }
   printf ("\n");
#endif
}

/* Compute a marking threshold from the spectrum X. */
void cc6_compute_masking(const struct cc6_PsyDecay *decay, cc6_celt_word16_t *X, cc6_celt_mask_t * __restrict mask, int len)
{
   int i;
   int N;
   N=len>>1;
   mask[0] = cc6_MULT16_16(X[0], X[0]);
   for (i=1;i<N;i++)
      mask[i] = cc6_ADD32(cc6_MULT16_16(X[i*2], X[i*2]), cc6_MULT16_16(X[i*2+1], X[i*2+1]));
   /* TODO: Do tone masking */
   /* Noise masking */
   cc6_spreading_func(decay, mask, N);
}

#ifdef EXP_PSY /* Not needed for now, but will be useful in the future */
void cc6_compute_mdct_masking(const struct cc6_PsyDecay *decay, cc6_celt_word32_t *X, cc6_celt_word16_t *tonality, cc6_celt_word16_t *long_window, cc6_celt_mask_t *mask, int len)
{
   int i;
   cc6_VARDECL(float, psd);
   cc6_SAVE_STACK;
   cc6_ALLOC(psd, len, float);
   for (i=0;i<len;i++)
      psd[i] = X[i]*X[i]*tonality[i];
   for (i=1;i<len-1;i++)
      mask[i] = .5*psd[i] + .25*(psd[i-1]+psd[i+1]);
   /*psd[0] = .5*mask[0]+.25*(mask[1]+mask[2]);*/
   mask[0] = .5*psd[0]+.5*psd[1];
   mask[len-1] = .5*(psd[len-1]+psd[len-2]);
   /* TODO: Do tone masking */
   /* Noise masking */
   cc6_spreading_func(decay, mask, len);
   cc6_RESTORE_STACK;
}

void cc6_compute_tonality(const cc6_CELTMode *m, cc6_celt_word16_t * restrict X, cc6_celt_word16_t * mem, int len, cc6_celt_word16_t *tonality, int mdct_size)
{
   int i;
   cc6_celt_word16_t norm_1;
   cc6_celt_word16_t *mem2;
   int N = len>>2;

   mem2 = mem+2*N;
   X[0] = 0;
   X[1] = 0;
   tonality[0] = 1;
   for (i=1;i<N;i++)
   {
      cc6_celt_word16_t re, im, re2, im2;
      re = X[2*i];
      im = X[2*i+1];
      /* Normalise spectrum */
      norm_1 = cc6_celt_rsqrt(.01+cc6_MAC16_16(cc6_MULT16_16(re,re), im,im));
      re = cc6_MULT16_16(re, norm_1);
      im = cc6_MULT16_16(im, norm_1);
      /* Phase derivative */
      re2 = re*mem[2*i] + im*mem[2*i+1];
      im2 = im*mem[2*i] - re*mem[2*i+1];
      mem[2*i] = re;
      mem[2*i+1] = im;
      /* Phase second derivative */
      re = re2*mem2[2*i] + im2*mem2[2*i+1];
      im = im2*mem2[2*i] - re2*mem2[2*i+1];
      mem2[2*i] = re2;
      mem2[2*i+1] = im2;
      /*printf ("%f ", re);*/
      X[2*i] = re;
      X[2*i+1] = im;
   }
   /*printf ("\n");*/
   for (i=0;i<mdct_size;i++)
   {
      tonality[i] = 1.0-X[2*i]*X[2*i]*X[2*i];
      if (tonality[i]>1)
         tonality[i] = 1;
      if (tonality[i]<.02)
         tonality[i]=.02;
   }
}
#endif
