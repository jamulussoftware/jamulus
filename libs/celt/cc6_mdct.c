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

/* This is a simple MDCT implementation that uses a N/4 complex FFT
   to do most of the work. It should be relatively straightforward to
   plug in pretty much and FFT here.
   
   This replaces the Vorbis FFT (and uses the exact same API), which 
   was a bit too messy and that was ending up duplicating code 
   (might as well use the same FFT everywhere).
   
   The algorithm is similar to (and inspired from) Fabrice Bellard's
   MDCT implementation in FFMPEG, but has differences in signs, ordering
   and scaling in many places. 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cc6_mdct.h"
#include "cc6_kfft_double.h"
#include <math.h>
#include "cc6_os_support.h"
#include "cc6_mathops.h"
#include "cc6_stack_alloc.h"

#ifndef cc6_M_PI
#define cc6_M_PI 3.141592653
#endif

void cc6_mdct_init(cc6_mdct_lookup *l,int N)
{
   int i;
   int N2;
   l->n = N;
   N2 = N>>1;
   l->kfft = cc6_cpx32_fft_alloc(N>>2);
   if (l->kfft==NULL)
     return;
   l->trig = (cc6_kiss_twiddle_scalar*)cc6_celt_alloc(N2*sizeof(cc6_kiss_twiddle_scalar));
   if (l->trig==NULL)
     return;
   /* We have enough points that sine isn't necessary */
#if defined(FIXED_POINT)
#if defined(DOUBLE_PRECISION) & !defined(MIXED_PRECISION)
   for (i=0;i<N2;i++)
      l->trig[i] = cc6_SAMP_MAX*cos(2*cc6_M_PI*(i+1./8.)/N);
#else
   for (i=0;i<N2;i++)
      l->trig[i] = cc6_TRIG_UPSCALE*cc6_celt_cos_norm(cc6_DIV32(cc6_ADD32(cc6_SHL32(cc6_EXTEND32(i),17),16386),N));
#endif
#else
   for (i=0;i<N2;i++)
      l->trig[i] = cos(2*cc6_M_PI*(i+1./8.)/N);
#endif
}

void cc6_mdct_clear(cc6_mdct_lookup *l)
{
   cc6_cpx32_fft_free(l->kfft);
   cc6_celt_free(l->trig);
}

void cc6_mdct_forward(const cc6_mdct_lookup *l, cc6_kiss_fft_scalar *in, cc6_kiss_fft_scalar * __restrict out, const cc6_celt_word16_t *window, int overlap)
{
   int i;
   int N, N2, N4;
   cc6_VARDECL(cc6_kiss_fft_scalar, f);
   cc6_SAVE_STACK;
   N = l->n;
   N2 = N>>1;
   N4 = N>>2;
   cc6_ALLOC(f, N2, cc6_kiss_fft_scalar);
   
   /* Consider the input to be compused of four blocks: [a, b, c, d] */
   /* Window, shuffle, fold */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const cc6_kiss_fft_scalar * __restrict xp1 = in+(overlap>>1);
      const cc6_kiss_fft_scalar * __restrict xp2 = in+N2-1+(overlap>>1);
      cc6_kiss_fft_scalar * __restrict yp = out;
      const cc6_celt_word16_t * __restrict wp1 = window+(overlap>>1);
      const cc6_celt_word16_t * __restrict wp2 = window+(overlap>>1)-1;
      for(i=0;i<(overlap>>2);i++)
      {
         /* Real part arranged as -d-cR, Imag part arranged as -b+aR*/
         *yp++ = cc6_MULT16_32_Q15(*wp2, xp1[N2]) + cc6_MULT16_32_Q15(*wp1,*xp2);
         *yp++ = cc6_MULT16_32_Q15(*wp1, *xp1)    - cc6_MULT16_32_Q15(*wp2, xp2[-N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
      wp1 = window;
      wp2 = window+overlap-1;
      for(;i<N4-(overlap>>2);i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ = *xp2;
         *yp++ = *xp1;
         xp1+=2;
         xp2-=2;
      }
      for(;i<N4;i++)
      {
         /* Real part arranged as a-bR, Imag part arranged as -c-dR */
         *yp++ =  -cc6_MULT16_32_Q15(*wp1, xp1[-N2]) + cc6_MULT16_32_Q15(*wp2, *xp2);
         *yp++ = cc6_MULT16_32_Q15(*wp2, *xp1)     + cc6_MULT16_32_Q15(*wp1, xp2[N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
   }
   /* Pre-rotation */
   {
      cc6_kiss_fft_scalar * __restrict yp = out;
      cc6_kiss_fft_scalar *t = &l->trig[0];
      for(i=0;i<N4;i++)
      {
         cc6_kiss_fft_scalar re, im;
         re = yp[0];
         im = yp[1];
         *yp++ = -cc6_S_MUL(re,t[0])  +  cc6_S_MUL(im,t[N4]);
         *yp++ = -cc6_S_MUL(im,t[0])  -  cc6_S_MUL(re,t[N4]);
         t++;
      }
   }

   /* N/4 complex FFT, down-scales by 4/N */
   cc6_cpx32_fft(l->kfft, out, f, N4);

   /* Post-rotate */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const cc6_kiss_fft_scalar * __restrict fp = f;
      cc6_kiss_fft_scalar * __restrict yp1 = out;
      cc6_kiss_fft_scalar * __restrict yp2 = out+N2-1;
      cc6_kiss_fft_scalar *t = &l->trig[0];
      /* Temp pointers to make it really clear to the compiler what we're doing */
      for(i=0;i<N4;i++)
      {
         *yp1 = -cc6_S_MUL(fp[1],t[N4]) + cc6_S_MUL(fp[0],t[0]);
         *yp2 = -cc6_S_MUL(fp[0],t[N4]) - cc6_S_MUL(fp[1],t[0]);
         fp += 2;
         yp1 += 2;
         yp2 -= 2;
         t++;
      }
   }
   cc6_RESTORE_STACK;
}


void cc6_mdct_backward(const cc6_mdct_lookup *l, cc6_kiss_fft_scalar *in, cc6_kiss_fft_scalar * __restrict out, const cc6_celt_word16_t * __restrict window, int overlap)
{
   int i;
   int N, N2, N4;
   cc6_VARDECL(cc6_kiss_fft_scalar, f);
   cc6_VARDECL(cc6_kiss_fft_scalar, f2);
   cc6_SAVE_STACK;
   N = l->n;
   N2 = N>>1;
   N4 = N>>2;
   cc6_ALLOC(f, N2, cc6_kiss_fft_scalar);
   cc6_ALLOC(f2, N2, cc6_kiss_fft_scalar);
   
   /* Pre-rotate */
   {
      /* Temp pointers to make it really clear to the compiler what we're doing */
      const cc6_kiss_fft_scalar * __restrict xp1 = in;
      const cc6_kiss_fft_scalar * __restrict xp2 = in+N2-1;
      cc6_kiss_fft_scalar * __restrict yp = f2;
      cc6_kiss_fft_scalar *t = &l->trig[0];
      for(i=0;i<N4;i++) 
      {
         *yp++ = -cc6_S_MUL(*xp2, t[0])  - cc6_S_MUL(*xp1,t[N4]);
         *yp++ =  cc6_S_MUL(*xp2, t[N4]) - cc6_S_MUL(*xp1,t[0]);
         xp1+=2;
         xp2-=2;
         t++;
      }
   }

   /* Inverse N/4 complex FFT. This one should *not* downscale even in fixed-point */
   cc6_cpx32_ifft(l->kfft, f2, f, N4);
   
   /* Post-rotate */
   {
      cc6_kiss_fft_scalar * __restrict fp = f;
      cc6_kiss_fft_scalar *t = &l->trig[0];

      for(i=0;i<N4;i++)
      {
         cc6_kiss_fft_scalar re, im;
         re = fp[0];
         im = fp[1];
         /* We'd scale up by 2 here, but instead it's done when mixing the windows */
         *fp++ = cc6_S_MUL(re,*t) + cc6_S_MUL(im,t[N4]);
         *fp++ = cc6_S_MUL(im,*t) - cc6_S_MUL(re,t[N4]);
         t++;
      }
   }
   /* De-shuffle the components for the middle of the window only */
   {
      const cc6_kiss_fft_scalar * __restrict fp1 = f;
      const cc6_kiss_fft_scalar * __restrict fp2 = f+N2-1;
      cc6_kiss_fft_scalar * __restrict yp = f2;
      for(i = 0; i < N4; i++)
      {
         *yp++ =-*fp1;
         *yp++ = *fp2;
         fp1 += 2;
         fp2 -= 2;
      }
   }

   /* Mirror on both sides for TDAC */
   {
      cc6_kiss_fft_scalar * __restrict fp1 = f2+N4-1;
      cc6_kiss_fft_scalar * __restrict xp1 = out+N2-1;
      cc6_kiss_fft_scalar * __restrict yp1 = out+N4-overlap/2;
      const cc6_celt_word16_t * __restrict wp1 = window;
      const cc6_celt_word16_t * __restrict wp2 = window+overlap-1;
      for(i = 0; i< N4-overlap/2; i++)
      {
         *xp1 = *fp1;
         xp1--;
         fp1--;
      }
      for(; i < N4; i++)
      {
         cc6_kiss_fft_scalar x1;
         x1 = *fp1--;
         *yp1++ +=-cc6_MULT16_32_Q15(*wp1, x1);
         *xp1-- += cc6_MULT16_32_Q15(*wp2, x1);
         wp1++;
         wp2--;
      }
   }
   {
      cc6_kiss_fft_scalar * __restrict fp2 = f2+N4;
      cc6_kiss_fft_scalar * __restrict xp2 = out+N2;
      cc6_kiss_fft_scalar * __restrict yp2 = out+N-1-(N4-overlap/2);
      const cc6_celt_word16_t * __restrict wp1 = window;
      const cc6_celt_word16_t * __restrict wp2 = window+overlap-1;
      for(i = 0; i< N4-overlap/2; i++)
      {
         *xp2 = *fp2;
         xp2++;
         fp2++;
      }
      for(; i < N4; i++)
      {
         cc6_kiss_fft_scalar x2;
         x2 = *fp2++;
         *yp2--  = cc6_MULT16_32_Q15(*wp1, x2);
         *xp2++  = cc6_MULT16_32_Q15(*wp2, x2);
         wp1++;
         wp2--;
      }
   }
   cc6_RESTORE_STACK;
}


