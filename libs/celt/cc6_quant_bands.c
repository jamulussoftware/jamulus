/* (C) 2007-2008 Jean-Marc Valin, CSIRO
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

#include "cc6_quant_bands.h"
#include "cc6_laplace.h"
#include <math.h>
#include "cc6_os_support.h"
#include "cc6_arch.h"
#include "cc6_mathops.h"
#include "cc6_stack_alloc.h"

#ifdef FIXED_POINT
const cc6_celt_word16_t cc6_eMeans[24] = {1920, -341, -512, -107, 43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
const cc6_celt_word16_t cc6_eMeans[24] = {7.5f, -1.33f, -2.f, -0.42f, 0.17f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
#endif

int cc6_intra_decision(cc6_celt_word16_t *eBands, cc6_celt_word16_t *oldEBands, int len)
{
   int i;
   cc6_celt_word32_t dist = 0;
   for (i=0;i<len;i++)
   {
      cc6_celt_word16_t d = cc6_SUB16(eBands[i], oldEBands[i]);
      dist = cc6_MAC16_16(dist, d,d);
   }
   return cc6_SHR32(dist,16) > 2*len;
}

int *cc6_quant_prob_alloc(const cc6_CELTMode *m)
{
   int i;
   int *prob;
   prob = cc6_celt_alloc(4*m->nbEBands*sizeof(int));
   if (prob==NULL)
     return NULL;
   for (i=0;i<m->nbEBands;i++)
   {
      prob[2*i] = 6000-i*200;
      prob[2*i+1] = cc6_ec_laplace_get_start_freq(prob[2*i]);
   }
   for (i=0;i<m->nbEBands;i++)
   {
      prob[2*m->nbEBands+2*i] = 9000-i*240;
      prob[2*m->nbEBands+2*i+1] = cc6_ec_laplace_get_start_freq(prob[2*m->nbEBands+2*i]);
   }
   return prob;
}

void cc6_quant_prob_free(int *freq)
{
   cc6_celt_free(freq);
}

unsigned cc6_quant_coarse_energy(const cc6_CELTMode *m, cc6_celt_word16_t *eBands, cc6_celt_word16_t *oldEBands, int budget, int intra, int *prob, cc6_celt_word16_t *error, cc6_ec_enc *enc)
{
   int i, c;
   unsigned bits;
   unsigned bits_used = 0;
   cc6_celt_word16_t prev[2] = {0,0};
   cc6_celt_word16_t coef = m->ePredCoef;
   cc6_celt_word16_t beta;
   const int C = cc6_CHANNELS(m);

   if (intra)
   {
      coef = 0;
      prob += 2*m->nbEBands;
   }
   /* The .8 is a heuristic */
   beta = cc6_MULT16_16_Q15(cc6_QCONST16(.8f,15),coef);
   
   bits = cc6_ec_enc_tell(enc, 0);
   /* Encode at a fixed coarse resolution */
   for (i=0;i<m->nbEBands;i++)
   {
      c=0;
      do {
         int qi;
         cc6_celt_word16_t q;   /* dB */
         cc6_celt_word16_t x;   /* dB */
         cc6_celt_word16_t f;   /* Q8 */
         cc6_celt_word16_t mean = cc6_MULT16_16_Q15(cc6_Q15ONE-coef,cc6_eMeans[i]);
         x = eBands[i+c*m->nbEBands];
#ifdef FIXED_POINT
         f = x-mean -cc6_MULT16_16_Q15(coef,oldEBands[i+c*m->nbEBands])-prev[c];
         /* Rounding to nearest integer here is really important! */
         qi = (f+128)>>8;
#else
         f = x-mean-coef*oldEBands[i+c*m->nbEBands]-prev[c];
         /* Rounding to nearest integer here is really important! */
         qi = (int)floor(.5f+f);
#endif
         /* If we don't have enough bits to encode all the energy, just assume something safe.
            We allow slightly busting the budget here */
         bits_used=cc6_ec_enc_tell(enc, 0) - bits;
         if (bits_used > budget)
         {
            qi = -1;
            error[i+c*m->nbEBands] = 128;
         } else {
            cc6_ec_laplace_encode_start(enc, &qi, prob[2*i], prob[2*i+1]);
            error[i+c*m->nbEBands] = f - cc6_SHL16(qi,8);
         }
         q = qi*cc6_DB_SCALING;
         
         oldEBands[i+c*m->nbEBands] = cc6_MULT16_16_Q15(coef,oldEBands[i+c*m->nbEBands])+(mean+prev[c]+q);
         prev[c] = mean+prev[c]+cc6_MULT16_16_Q15(cc6_Q15ONE-beta,q);
      } while (++c < C);
   }
   return bits_used;
}

void cc6_quant_fine_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, cc6_celt_word16_t *error, int *fine_quant, cc6_ec_enc *enc)
{
   int i, c;
   const int C = cc6_CHANNELS(m);

   /* Encode finer resolution */
   for (i=0;i<m->nbEBands;i++)
   {
      cc6_celt_int16_t frac = 1<<fine_quant[i];
      if (fine_quant[i] <= 0)
         continue;
      c=0;
      do {
         int q2;
         cc6_celt_word16_t offset;
#ifdef FIXED_POINT
         /* Has to be without rounding */
         q2 = (error[i+c*m->nbEBands]+cc6_QCONST16(.5f,8))>>(8-fine_quant[i]);
#else
         q2 = (int)floor((error[i+c*m->nbEBands]+.5f)*frac);
#endif
         if (q2 > frac-1)
            q2 = frac-1;
         cc6_ec_enc_bits(enc, q2, fine_quant[i]);
#ifdef FIXED_POINT
         offset = cc6_SUB16(cc6_SHR16(cc6_SHL16(q2,8)+cc6_QCONST16(.5,8),fine_quant[i]),cc6_QCONST16(.5f,8));
#else
         offset = (q2+.5f)*(1<<(14-fine_quant[i]))*(1.f/16384) - .5f;
#endif
         oldEBands[i+c*m->nbEBands] += offset;
         error[i+c*m->nbEBands] -= offset;
         eBands[i+c*m->nbEBands] = cc6_log2Amp(oldEBands[i+c*m->nbEBands]);
         /*printf ("%f ", error[i] - offset);*/
      } while (++c < C);
   }
   for (i=0;i<C*m->nbEBands;i++)
      eBands[i] = cc6_log2Amp(oldEBands[i]);
}

void cc6_quant_energy_finalise(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, cc6_celt_word16_t *error, int *fine_quant, int *fine_priority, int bits_left, cc6_ec_enc *enc)
{
   int i, prio, c;
   const int C = cc6_CHANNELS(m);

   /* Use up the remaining bits */
   for (prio=0;prio<2;prio++)
   {
      for (i=0;i<m->nbEBands && bits_left>=C ;i++)
      {
         if (fine_quant[i] >= 7 || fine_priority[i]!=prio)
            continue;
         c=0;
         do {
            int q2;
            cc6_celt_word16_t offset;
            q2 = error[i+c*m->nbEBands]<0 ? 0 : 1;
            cc6_ec_enc_bits(enc, q2, 1);
#ifdef FIXED_POINT
            offset = cc6_SHR16(cc6_SHL16(q2,8)-cc6_QCONST16(.5,8),fine_quant[i]+1);
#else
            offset = (q2-.5f)*(1<<(14-fine_quant[i]-1))*(1.f/16384);
#endif
            oldEBands[i+c*m->nbEBands] += offset;
            bits_left--;
         } while (++c < C);
      }
   }
   for (i=0;i<C*m->nbEBands;i++)
   {
      eBands[i] = cc6_log2Amp(oldEBands[i]);
      if (oldEBands[i] < -cc6_QCONST16(7.f,8))
         oldEBands[i] = -cc6_QCONST16(7.f,8);
   }
}

void cc6_unquant_coarse_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int budget, int intra, int *prob, cc6_ec_dec *dec)
{
   int i, c;
   unsigned bits;
   cc6_celt_word16_t prev[2] = {0, 0};
   cc6_celt_word16_t coef = m->ePredCoef;
   cc6_celt_word16_t beta;
   const int C = cc6_CHANNELS(m);

   if (intra)
   {
      coef = 0;
      prob += 2*m->nbEBands;
   }
   /* The .8 is a heuristic */
   beta = cc6_MULT16_16_Q15(cc6_QCONST16(.8f,15),coef);
   
   bits = cc6_ec_dec_tell(dec, 0);
   /* Decode at a fixed coarse resolution */
   for (i=0;i<m->nbEBands;i++)
   {
      c=0; 
      do {
         int qi;
         cc6_celt_word16_t q;
         cc6_celt_word16_t mean = cc6_MULT16_16_Q15(cc6_Q15ONE-coef,cc6_eMeans[i]);
         /* If we didn't have enough bits to encode all the energy, just assume something safe.
            We allow slightly busting the budget here */
         if (cc6_ec_dec_tell(dec, 0) - bits > budget)
            qi = -1;
         else
            qi = cc6_ec_laplace_decode_start(dec, prob[2*i], prob[2*i+1]);
         q = qi*cc6_DB_SCALING;

         oldEBands[i+c*m->nbEBands] = cc6_MULT16_16_Q15(coef,oldEBands[i+c*m->nbEBands])+(mean+prev[c]+q);
         prev[c] = mean+prev[c]+cc6_MULT16_16_Q15(cc6_Q15ONE-beta,q);
      } while (++c < C);
   }
}

void cc6_unquant_fine_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int *fine_quant, cc6_ec_dec *dec)
{
   int i, c;
   const int C = cc6_CHANNELS(m);
   /* Decode finer resolution */
   for (i=0;i<m->nbEBands;i++)
   {
      if (fine_quant[i] <= 0)
         continue;
      c=0; 
      do {
         int q2;
         cc6_celt_word16_t offset;
         q2 = cc6_ec_dec_bits(dec, fine_quant[i]);
#ifdef FIXED_POINT
         offset = cc6_SUB16(cc6_SHR16(cc6_SHL16(q2,8)+cc6_QCONST16(.5,8),fine_quant[i]),cc6_QCONST16(.5f,8));
#else
         offset = (q2+.5f)*(1<<(14-fine_quant[i]))*(1.f/16384) - .5f;
#endif
         oldEBands[i+c*m->nbEBands] += offset;
      } while (++c < C);
   }
   for (i=0;i<C*m->nbEBands;i++)
      eBands[i] = cc6_log2Amp(oldEBands[i]);
}

void cc6_unquant_energy_finalise(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int *fine_quant,  int *fine_priority, int bits_left, cc6_ec_dec *dec)
{
   int i, prio, c;
   const int C = cc6_CHANNELS(m);

   /* Use up the remaining bits */
   for (prio=0;prio<2;prio++)
   {
      for (i=0;i<m->nbEBands && bits_left>=C ;i++)
      {
         if (fine_quant[i] >= 7 || fine_priority[i]!=prio)
            continue;
         c=0;
         do {
            int q2;
            cc6_celt_word16_t offset;
            q2 = cc6_ec_dec_bits(dec, 1);
#ifdef FIXED_POINT
            offset = cc6_SHR16(cc6_SHL16(q2,8)-cc6_QCONST16(.5,8),fine_quant[i]+1);
#else
            offset = (q2-.5f)*(1<<(14-fine_quant[i]-1))*(1.f/16384);
#endif
            oldEBands[i+c*m->nbEBands] += offset;
            bits_left--;
         } while (++c < C);
      }
   }
   for (i=0;i<C*m->nbEBands;i++)
   {
      eBands[i] = cc6_log2Amp(oldEBands[i]);
      if (oldEBands[i] < -cc6_QCONST16(7.f,8))
         oldEBands[i] = -cc6_QCONST16(7.f,8);
   }
}
