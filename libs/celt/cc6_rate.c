/* (C) 2007-2009 Jean-Marc Valin, CSIRO
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

#include <math.h>
#include "cc6_modes.h"
#include "cc6_cwrs.h"
#include "cc6_arch.h"
#include "cc6_os_support.h"

#include "cc6_entcode.h"
#include "cc6_rate.h"


#ifndef STATIC_MODES

cc6_celt_int16_t **cc6_compute_alloc_cache(cc6_CELTMode *m, int C)
{
   int i, prevN;
   int error = 0;
   cc6_celt_int16_t **bits;
   const cc6_celt_int16_t *eBands = m->eBands;

   bits = cc6_celt_alloc(m->nbEBands*sizeof(cc6_celt_int16_t*));
   if (bits==NULL)
     return NULL;
        
   prevN = -1;
   for (i=0;i<m->nbEBands;i++)
   {
      int N = C*(eBands[i+1]-eBands[i]);
      if (N == prevN && eBands[i] < m->pitchEnd)
      {
         bits[i] = bits[i-1];
      } else {
         bits[i] = cc6_celt_alloc(cc6_MAX_PULSES*sizeof(cc6_celt_int16_t));
         if (bits[i]!=NULL) {
           cc6_get_required_bits(bits[i], N, cc6_MAX_PULSES, cc6_BITRES);
         } else {
            error=1;
         }
         prevN = N;
      }
   }
   if (error)
   {
      const cc6_celt_int16_t *prevPtr = NULL;
      if (bits!=NULL)
      {
         for (i=0;i<m->nbEBands;i++)
         {
            if (bits[i] != prevPtr)
            {
               prevPtr = bits[i];
               cc6_celt_free((int*)bits[i]);
            }
         }
      free(bits);
      bits=NULL;
      }   
   }
   return bits;
}

#endif /* !STATIC_MODES */



static void cc6_interp_bits2pulses(const cc6_CELTMode *m, int *bits1, int *bits2, int total, int *bits, int *ebits, int *fine_priority, int len)
{
   int psum;
   int lo, hi;
   int j;
   const int C = cc6_CHANNELS(m);
   cc6_SAVE_STACK;
   lo = 0;
   hi = 1<<cc6_BITRES;
   while (hi-lo != 1)
   {
      int mid = (lo+hi)>>1;
      psum = 0;
      for (j=0;j<len;j++)
         psum += ((1<<cc6_BITRES)-mid)*bits1[j] + mid*bits2[j];
      if (psum > (total<<cc6_BITRES))
         hi = mid;
      else
         lo = mid;
   }
   psum = 0;
   /*printf ("interp bisection gave %d\n", lo);*/
   for (j=0;j<len;j++)
   {
      bits[j] = ((1<<cc6_BITRES)-lo)*bits1[j] + lo*bits2[j];
      psum += bits[j];
   }
   /* Allocate the remaining bits */
   {
      int left, perband;
      left = (total<<cc6_BITRES)-psum;
      perband = left/len;
      for (j=0;j<len;j++)
         bits[j] += perband;
      left = left-len*perband;
      for (j=0;j<left;j++)
         bits[j]++;
   }
   for (j=0;j<len;j++)
   {
      int N, d;
      int offset;

      N=m->eBands[j+1]-m->eBands[j]; 
      d=C*N<<cc6_BITRES;
      offset = 50 - cc6_log2_frac(N, 4);
      /* Offset for the number of fine bits compared to their "fair share" of total/N */
      offset = bits[j]-offset*N*C;
      if (offset < 0)
         offset = 0;
      ebits[j] = (2*offset+d)/(2*d);
      fine_priority[j] = ebits[j]*d >= offset;

      /* Make sure not to bust */
      if (C*ebits[j] > (bits[j]>>cc6_BITRES))
         ebits[j] = bits[j]/C >> cc6_BITRES;

      if (ebits[j]>7)
         ebits[j]=7;
      /* The bits used for fine allocation can't be used for pulses */
      bits[j] -= C*ebits[j]<<cc6_BITRES;
      if (bits[j] < 0)
         bits[j] = 0;
   }
   cc6_RESTORE_STACK;
}

void cc6_compute_allocation(const cc6_CELTMode *m, int *offsets, int total, int *pulses, int *ebits, int *fine_priority)
{
   int lo, hi, len, j;
   cc6_VARDECL(int, bits1);
   cc6_VARDECL(int, bits2);
   cc6_SAVE_STACK;
   
   len = m->nbEBands;
   cc6_ALLOC(bits1, len, int);
   cc6_ALLOC(bits2, len, int);

   lo = 0;
   hi = m->nbAllocVectors - 1;
   while (hi-lo != 1)
   {
      int psum = 0;
      int mid = (lo+hi) >> 1;
      for (j=0;j<len;j++)
      {
         bits1[j] = (m->allocVectors[mid*len+j] + offsets[j])<<cc6_BITRES;
         if (bits1[j] < 0)
            bits1[j] = 0;
         psum += bits1[j];
         /*printf ("%d ", bits[j]);*/
      }
      /*printf ("\n");*/
      if (psum > (total<<cc6_BITRES))
         hi = mid;
      else
         lo = mid;
      /*printf ("lo = %d, hi = %d\n", lo, hi);*/
   }
   /*printf ("interp between %d and %d\n", lo, hi);*/
   for (j=0;j<len;j++)
   {
      bits1[j] = m->allocVectors[lo*len+j] + offsets[j];
      bits2[j] = m->allocVectors[hi*len+j] + offsets[j];
      if (bits1[j] < 0)
         bits1[j] = 0;
      if (bits2[j] < 0)
         bits2[j] = 0;
   }
   cc6_interp_bits2pulses(m, bits1, bits2, total, pulses, ebits, fine_priority, len);
   cc6_RESTORE_STACK;
}

