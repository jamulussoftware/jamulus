/* (C) 2007-2008 Jean-Marc Valin, CSIRO
   (C) 2008-2009 Gregory Maxwell */
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
#include "cc6_bands.h"
#include "cc6_modes.h"
#include "cc6_vq.h"
#include "cc6_cwrs.h"
#include "cc6_stack_alloc.h"
#include "cc6_os_support.h"
#include "cc6_mathops.h"
#include "cc6_rate.h"

const cc6_celt_word16_t sqrtC_1[2] = {cc6_QCONST16(1.f, 14), cc6_QCONST16(1.414214f, 14)};

#ifdef FIXED_POINT
/* Compute the amplitude (sqrt energy) in each of the bands */
void cc6_compute_band_energies(const cc6_CELTMode *m, const cc6_celt_sig_t *X, cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   for (c=0;c<C;c++)
   {
      for (i=0;i<m->nbEBands;i++)
      {
         int j;
         cc6_celt_word32_t maxval=0;
         cc6_celt_word32_t sum = 0;
         
         j=eBands[i]; do {
            maxval = cc6_MAX32(maxval, X[j+c*N]);
            maxval = cc6_MAX32(maxval, -X[j+c*N]);
         } while (++j<eBands[i+1]);
         
         if (maxval > 0)
         {
            int shift = cc6_celt_ilog2(maxval)-10;
            j=eBands[i]; do {
               sum = cc6_MAC16_16(sum, cc6_EXTRACT16(cc6_VSHR32(X[j+c*N],shift)),
                                   cc6_EXTRACT16(cc6_VSHR32(X[j+c*N],shift)));
            } while (++j<eBands[i+1]);
            /* We're adding one here to make damn sure we never end up with a pitch vector that's
               larger than unity norm */
            bank[i+c*m->nbEBands] = cc6_EPSILON+cc6_VSHR32(cc6_EXTEND32(cc6_celt_sqrt(sum)),-shift);
         } else {
            bank[i+c*m->nbEBands] = cc6_EPSILON;
         }
         /*printf ("%f ", bank[i+c*m->nbEBands]);*/
      }
   }
   /*printf ("\n");*/
}

/* Normalise each band such that the energy is one. */
void cc6_normalise_bands(const cc6_CELTMode *m, const cc6_celt_sig_t * __restrict freq, cc6_celt_norm_t * __restrict X, const cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   for (c=0;c<C;c++)
   {
      i=0; do {
         cc6_celt_word16_t g;
         int j,shift;
         cc6_celt_word16_t E;
         shift = cc6_celt_zlog2(bank[i+c*m->nbEBands])-13;
         E = cc6_VSHR32(bank[i+c*m->nbEBands], shift);
         g = cc6_EXTRACT16(cc6_celt_rcp(cc6_SHL32(E,3)));
         j=eBands[i]; do {
            X[j*C+c] = cc6_MULT16_16_Q15(cc6_VSHR32(freq[j+c*N],shift-1),g);
         } while (++j<eBands[i+1]);
      } while (++i<m->nbEBands);
   }
}

#else /* FIXED_POINT */
/* Compute the amplitude (sqrt energy) in each of the bands */
void cc6_compute_band_energies(const cc6_CELTMode *m, const cc6_celt_sig_t *X, cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   for (c=0;c<C;c++)
   {
      for (i=0;i<m->nbEBands;i++)
      {
         int j;
         cc6_celt_word32_t sum = 1e-10;
         for (j=eBands[i];j<eBands[i+1];j++)
            sum += X[j+c*N]*X[j+c*N];
         bank[i+c*m->nbEBands] = sqrt(sum);
         /*printf ("%f ", bank[i+c*m->nbEBands]);*/
      }
   }
   /*printf ("\n");*/
}

#ifdef EXP_PSY
void cc6_compute_noise_energies(const cc6_CELTMode *m, const cc6_celt_sig_t *X, const cc6_celt_word16_t *tonality, cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   for (c=0;c<C;c++)
   {
      for (i=0;i<m->nbEBands;i++)
      {
         int j;
         cc6_celt_word32_t sum = 1e-10;
         for (j=eBands[i];j<eBands[i+1];j++)
            sum += X[j*C+c]*X[j+c*N]*tonality[j];
         bank[i+c*m->nbEBands] = sqrt(sum);
         /*printf ("%f ", bank[i+c*m->nbEBands]);*/
      }
   }
   /*printf ("\n");*/
}
#endif

/* Normalise each band such that the energy is one. */
void cc6_normalise_bands(const cc6_CELTMode *m, const cc6_celt_sig_t * __restrict freq, cc6_celt_norm_t * __restrict X, const cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   for (c=0;c<C;c++)
   {
      for (i=0;i<m->nbEBands;i++)
      {
         int j;
         cc6_celt_word16_t g = 1.f/(1e-10+bank[i+c*m->nbEBands]);
         for (j=eBands[i];j<eBands[i+1];j++)
            X[j*C+c] = freq[j+c*N]*g;
      }
   }
}

#endif /* FIXED_POINT */

#ifndef DISABLE_STEREO
void cc6_renormalise_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X)
{
   int i, c;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   for (c=0;c<C;c++)
   {
      i=0; do {
         cc6_renormalise_vector(X+C*eBands[i]+c, cc6_QCONST16(0.70711f, 15), eBands[i+1]-eBands[i], C);
      } while (++i<m->nbEBands);
   }
}
#endif /* DISABLE_STEREO */

/* De-normalise the energy to produce the synthesis from the unit-energy bands */
void cc6_denormalise_bands(const cc6_CELTMode *m, const cc6_celt_norm_t * __restrict X, cc6_celt_sig_t * __restrict freq, const cc6_celt_ener_t *bank)
{
   int i, c, N;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   N = cc6_FRAMESIZE(m);
   if (C>2)
      cc6_celt_fatal("cc6_denormalise_bands() not implemented for >2 channels");
   for (c=0;c<C;c++)
   {
      for (i=0;i<m->nbEBands;i++)
      {
         int j;
         cc6_celt_word32_t g = cc6_SHR32(bank[i+c*m->nbEBands],1);
         j=eBands[i]; do {
            freq[j+c*N] = cc6_SHL32(cc6_MULT16_32_Q15(X[j*C+c], g),2);
         } while (++j<eBands[i+1]);
      }
      for (i=eBands[m->nbEBands];i<eBands[m->nbEBands+1];i++)
         freq[i+c*N] = 0;
   }
}


/* Compute the best gain for each "pitch band" */
int cc6_compute_pitch_gain(const cc6_CELTMode *m, const cc6_celt_norm_t *X, const cc6_celt_norm_t *P, cc6_celt_pgain_t *gains)
{
   int i;
   int gain_sum = 0;
   const cc6_celt_int16_t *pBands = m->pBands;
   const int C = cc6_CHANNELS(m);

   for (i=0;i<m->nbPBands;i++)
   {
      cc6_celt_word32_t Sxy=0, Sxx=0;
      int j;
      /* We know we're not going to overflow because Sxx can't be more than 1 (Q28) */
      for (j=C*pBands[i];j<C*pBands[i+1];j++)
      {
         Sxy = cc6_MAC16_16(Sxy, X[j], P[j]);
         Sxx = cc6_MAC16_16(Sxx, X[j], X[j]);
      }
      Sxy = cc6_SHR32(Sxy,2);
      Sxx = cc6_SHR32(Sxx,2);
      /* No negative gain allowed */
      if (Sxy < 0)
         Sxy = 0;
      /* Not sure how that would happen, just making sure */
      if (Sxy > Sxx)
         Sxy = Sxx;
      /* We need to be a bit conservative (multiply gain by 0.9), otherwise the
         residual doesn't quantise well */
      Sxy = cc6_MULT16_32_Q15(cc6_QCONST16(.99f, 15), Sxy);
      /* gain = Sxy/Sxx */
      gains[i] = cc6_EXTRACT16(cc6_celt_div(Sxy,cc6_ADD32(cc6_SHR32(Sxx, cc6_PGAIN_SHIFT),cc6_EPSILON)));
      if (gains[i]>cc6_QCONST16(.5,15))
         gain_sum++;
   }
   return gain_sum > 5;
}

#ifndef DISABLE_STEREO

static void cc6_stereo_band_mix(const cc6_CELTMode *m, cc6_celt_norm_t *X, const cc6_celt_ener_t *bank, int stereo_mode, int bandID, int dir)
{
   int i = bandID;
   const cc6_celt_int16_t *eBands = m->eBands;
   const int C = cc6_CHANNELS(m);
   int j;
   cc6_celt_word16_t a1, a2;
   if (stereo_mode==0)
   {
      /* Do mid-side when not doing intensity stereo */
      a1 = cc6_QCONST16(.70711f,14);
      a2 = dir*cc6_QCONST16(.70711f,14);
   } else {
      cc6_celt_word16_t left, right;
      cc6_celt_word16_t norm;
#ifdef FIXED_POINT
      int shift = cc6_celt_zlog2(cc6_MAX32(bank[i], bank[i+m->nbEBands]))-13;
#endif
      left = cc6_VSHR32(bank[i],shift);
      right = cc6_VSHR32(bank[i+m->nbEBands],shift);
      norm = cc6_EPSILON + cc6_celt_sqrt(cc6_EPSILON+cc6_MULT16_16(left,left)+cc6_MULT16_16(right,right));
      a1 = cc6_DIV32_16(cc6_SHL32(cc6_EXTEND32(left),14),norm);
      a2 = dir*cc6_DIV32_16(cc6_SHL32(cc6_EXTEND32(right),14),norm);
   }
   for (j=eBands[i];j<eBands[i+1];j++)
   {
      cc6_celt_norm_t r, l;
      l = X[j*C];
      r = X[j*C+1];
      X[j*C] = cc6_MULT16_16_Q14(a1,l) + cc6_MULT16_16_Q14(a2,r);
      X[j*C+1] = cc6_MULT16_16_Q14(a1,r) - cc6_MULT16_16_Q14(a2,l);
   }
}


void cc6_interleave(cc6_celt_norm_t *x, int N)
{
   int i;
   cc6_VARDECL(cc6_celt_norm_t, tmp);
   cc6_SAVE_STACK;
   cc6_ALLOC(tmp, N, cc6_celt_norm_t);
   
   for (i=0;i<N;i++)
      tmp[i] = x[i];
   for (i=0;i<N>>1;i++)
   {
      x[i<<1] = tmp[i];
      x[(i<<1)+1] = tmp[i+(N>>1)];
   }
   cc6_RESTORE_STACK;
}

void cc6_deinterleave(cc6_celt_norm_t *x, int N)
{
   int i;
   cc6_VARDECL(cc6_celt_norm_t, tmp);
   cc6_SAVE_STACK;
   cc6_ALLOC(tmp, N, cc6_celt_norm_t);
   
   for (i=0;i<N;i++)
      tmp[i] = x[i];
   for (i=0;i<N>>1;i++)
   {
      x[i] = tmp[i<<1];
      x[i+(N>>1)] = tmp[(i<<1)+1];
   }
   cc6_RESTORE_STACK;
}

#endif /* DISABLE_STEREO */

int cc6_folding_decision(const cc6_CELTMode *m, cc6_celt_norm_t *X, cc6_celt_word16_t *average, int *last_decision)
{
   int i;
   int NR=0;
   cc6_celt_word32_t ratio = cc6_EPSILON;
   const cc6_celt_int16_t * __restrict eBands = m->eBands;
   for (i=0;i<m->nbEBands;i++)
   {
      int j, N;
      int max_i=0;
      cc6_celt_word16_t max_val=cc6_EPSILON;
      cc6_celt_word32_t floor_ener=cc6_EPSILON;
      cc6_celt_norm_t * __restrict x = X+eBands[i];
      N = eBands[i+1]-eBands[i];
      for (j=0;j<N;j++)
      {
         if (cc6_ABS16(x[j])>max_val)
         {
            max_val = cc6_ABS16(x[j]);
            max_i = j;
         }
      }
#if 0
      for (j=0;j<N;j++)
      {
         if (abs(j-max_i)>2)
            floor_ener += x[j]*x[j];
      }
#else
      floor_ener = cc6_QCONST32(1.,28)-cc6_MULT16_16(max_val,max_val);
      if (max_i < N-1)
         floor_ener -= cc6_MULT16_16(x[max_i+1], x[max_i+1]);
      if (max_i < N-2)
         floor_ener -= cc6_MULT16_16(x[max_i+2], x[max_i+2]);
      if (max_i > 0)
         floor_ener -= cc6_MULT16_16(x[max_i-1], x[max_i-1]);
      if (max_i > 1)
         floor_ener -= cc6_MULT16_16(x[max_i-2], x[max_i-2]);
      floor_ener = cc6_MAX32(floor_ener, cc6_EPSILON);
#endif
      if (N>7 && eBands[i] >= m->pitchEnd)
      {
         cc6_celt_word16_t r;
         cc6_celt_word16_t den = cc6_celt_sqrt(floor_ener);
         den = cc6_MAX32(cc6_QCONST16(.02, 15), den);
         r = cc6_DIV32_16(cc6_SHL32(cc6_EXTEND32(max_val),8),den);
         ratio = cc6_ADD32(ratio, cc6_EXTEND32(r));
         NR++;
      }
   }
   if (NR>0)
      ratio = cc6_DIV32_16(ratio, NR);
   ratio = cc6_ADD32(cc6_HALF32(ratio), cc6_HALF32(*average));
   if (!*last_decision)
   {
      *last_decision = (ratio < cc6_QCONST16(1.8,8));
   } else {
      *last_decision = (ratio < cc6_QCONST16(3.,8));
   }
   *average = cc6_EXTRACT16(ratio);
   return *last_decision;
}

/* Quantisation of the residual */
void cc6_quant_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, cc6_celt_mask_t *W, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int shortBlocks, int fold, int total_bits, cc6_ec_enc *enc)
{
   int i, j, remaining_bits, balance;
   const cc6_celt_int16_t * __restrict eBands = m->eBands;
   cc6_celt_norm_t * __restrict norm;
   cc6_VARDECL(cc6_celt_norm_t, _norm);   const cc6_celt_int16_t *pBands = m->pBands;
   int pband=-1;
   int B;
   cc6_SAVE_STACK;

   B = shortBlocks ? m->nbShortMdcts : 1;
   cc6_ALLOC(_norm, eBands[m->nbEBands+1], cc6_celt_norm_t);
   norm = _norm;

   balance = 0;
   for (i=0;i<m->nbEBands;i++)
   {
      int tell;
      int N;
      int q;
      cc6_celt_word16_t n;
      const cc6_celt_int16_t * const *BPbits;
      
      int curr_balance, curr_bits;
      
      N = eBands[i+1]-eBands[i];
      BPbits = m->bits;

      tell = cc6_ec_enc_tell(enc, 4);
      if (i != 0)
         balance -= tell;
      remaining_bits = (total_bits<<cc6_BITRES)-tell-1;
      curr_balance = (m->nbEBands-i);
      if (curr_balance > 3)
         curr_balance = 3;
      curr_balance = balance / curr_balance;
      q = cc6_bits2pulses(m, BPbits[i], N, pulses[i]+curr_balance);
      curr_bits = cc6_pulses2bits(BPbits[i], N, q);
      remaining_bits -= curr_bits;
      while (remaining_bits < 0 && q > 0)
      {
         remaining_bits += curr_bits;
         q--;
         curr_bits = cc6_pulses2bits(BPbits[i], N, q);
         remaining_bits -= curr_bits;
      }
      balance += pulses[i] + tell;
      
      n = cc6_SHL16(cc6_celt_sqrt(eBands[i+1]-eBands[i]),11);

      /* If pitch is in use and this eBand begins a pitch band, encode the pitch gain flag */
      if (pitch_used && eBands[i]< m->pitchEnd && eBands[i] == pBands[pband+1])
      {
         int enabled = 1;
         pband++;
         if (remaining_bits >= 1<<cc6_BITRES) {
           enabled = pgains[pband] > cc6_QCONST16(.5,15);
           cc6_ec_enc_bits(enc, enabled, 1);
           balance += 1<<cc6_BITRES;
         }
         if (enabled)
            pgains[pband] = cc6_QCONST16(.9,15);
         else
            pgains[pband] = 0;
      }

      /* If pitch isn't available, use intra-frame prediction */
      if ((eBands[i] >= m->pitchEnd && fold) || q<=0)
      {
         cc6_intra_fold(m, X+eBands[i], eBands[i+1]-eBands[i], &q, norm, P+eBands[i], eBands[i], B);
      } else if (pitch_used && eBands[i] < m->pitchEnd) {
         for (j=eBands[i];j<eBands[i+1];j++)
            P[j] = cc6_MULT16_16_Q15(pgains[pband], P[j]);
      } else {
         for (j=eBands[i];j<eBands[i+1];j++)
            P[j] = 0;
      }
      
      if (q > 0)
      {
         cc6_alg_quant(X+eBands[i], W+eBands[i], eBands[i+1]-eBands[i], q, P+eBands[i], enc);
      } else {
         for (j=eBands[i];j<eBands[i+1];j++)
            X[j] = P[j];
      }
      for (j=eBands[i];j<eBands[i+1];j++)
         norm[j] = cc6_MULT16_16_Q15(n,X[j]);
   }
   cc6_RESTORE_STACK;
}

#ifndef DISABLE_STEREO

void cc6_quant_bands_stereo(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, cc6_celt_mask_t *W, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int shortBlocks, int fold, int total_bits, cc6_ec_enc *enc)
{
   int i, j, remaining_bits, balance;
   const cc6_celt_int16_t * __restrict eBands = m->eBands;
   cc6_celt_norm_t * __restrict norm;
   cc6_VARDECL(cc6_celt_norm_t, _norm);
   const int C = cc6_CHANNELS(m);
   const cc6_celt_int16_t *pBands = m->pBands;
   int pband=-1;
   int B;
   cc6_celt_word16_t mid, side;
   cc6_SAVE_STACK;

   B = shortBlocks ? m->nbShortMdcts : 1;
   cc6_ALLOC(_norm, C*eBands[m->nbEBands+1], cc6_celt_norm_t);
   norm = _norm;

   balance = 0;
   for (i=0;i<m->nbEBands;i++)
   {
      int c;
      int tell;
      int q1, q2;
      cc6_celt_word16_t n;
      const cc6_celt_int16_t * const *BPbits;
      int b, qb;
      int N;
      int curr_balance, curr_bits;
      int imid, iside, itheta;
      int mbits, sbits, delta;
      int qalloc;
      
      BPbits = m->bits;

      N = eBands[i+1]-eBands[i];
      tell = cc6_ec_enc_tell(enc, 4);
      if (i != 0)
         balance -= tell;
      remaining_bits = (total_bits<<cc6_BITRES)-tell-1;
      curr_balance = (m->nbEBands-i);
      if (curr_balance > 3)
         curr_balance = 3;
      curr_balance = balance / curr_balance;
      b = cc6_IMIN(remaining_bits+1,pulses[i]+curr_balance);
      if (b<0)
         b = 0;

      qb = (b-2*(N-1)*(40-cc6_log2_frac(N,4)))/(32*(N-1));
      if (qb > (b>>cc6_BITRES)-1)
         qb = (b>>cc6_BITRES)-1;
      if (qb<0)
         qb = 0;
      if (qb>14)
         qb = 14;

      cc6_stereo_band_mix(m, X, bandE, qb==0, i, 1);

      mid = cc6_renormalise_vector(X+C*eBands[i], cc6_Q15ONE, N, C);
      side = cc6_renormalise_vector(X+C*eBands[i]+1, cc6_Q15ONE, N, C);
#ifdef FIXED_POINT
      itheta = cc6_MULT16_16_Q15(cc6_QCONST16(0.63662,15),cc6_celt_atan2p(side, mid));
#else
      itheta = floor(.5+16384*0.63662*atan2(side,mid));
#endif
      qalloc = cc6_log2_frac((1<<qb)+1,4);
      if (qb==0)
      {
         itheta=0;
      } else {
         int shift;
         shift = 14-qb;
         itheta = (itheta+(1<<shift>>1))>>shift;
         cc6_ec_enc_uint(enc, itheta, (1<<qb)+1);
         itheta <<= shift;
      }
      if (itheta == 0)
      {
         imid = 32767;
         iside = 0;
         delta = -10000;
      } else if (itheta == 16384)
      {
         imid = 0;
         iside = 32767;
         delta = 10000;
      } else {
         imid = cc6_bitexact_cos(itheta);
         iside = cc6_bitexact_cos(16384-itheta);
         delta = (N-1)*(cc6_log2_frac(iside,6)-cc6_log2_frac(imid,6))>>2;
      }
      mbits = (b-qalloc/2-delta)/2;
      if (mbits > b-qalloc)
         mbits = b-qalloc;
      if (mbits<0)
         mbits=0;
      sbits = b-qalloc-mbits;
      q1 = cc6_bits2pulses(m, BPbits[i], N, mbits);
      q2 = cc6_bits2pulses(m, BPbits[i], N, sbits);
      curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
      remaining_bits -= curr_bits;
      while (remaining_bits < 0 && (q1 > 0 || q2 > 0))
      {
         remaining_bits += curr_bits;
         if (q1>q2)
         {
            q1--;
            curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
         } else {
            q2--;
            curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
         }
         remaining_bits -= curr_bits;
      }
      balance += pulses[i] + tell;
      
      n = cc6_SHL16(cc6_celt_sqrt((eBands[i+1]-eBands[i])),11);

      /* If pitch is in use and this eBand begins a pitch band, encode the pitch gain flag */
      if (pitch_used && eBands[i]< m->pitchEnd && eBands[i] == pBands[pband+1])
      {
         int enabled = 1;
         pband++;
         if (remaining_bits >= 1<<cc6_BITRES) {
            enabled = pgains[pband] > cc6_QCONST16(.5,15);
            cc6_ec_enc_bits(enc, enabled, 1);
            balance += 1<<cc6_BITRES;
         }
         if (enabled)
            pgains[pband] = cc6_QCONST16(.9,15);
         else
            pgains[pband] = 0;
      }
      

      /* If pitch isn't available, use intra-frame prediction */
      if ((eBands[i] >= m->pitchEnd && fold) || (q1+q2)<=0)
      {
         int K[2] = {q1, q2};
         cc6_intra_fold(m, X+C*eBands[i], eBands[i+1]-eBands[i], K, norm, P+C*eBands[i], eBands[i], B);
         cc6_deinterleave(P+C*eBands[i], C*N);
      } else if (pitch_used && eBands[i] < m->pitchEnd) {
         cc6_stereo_band_mix(m, P, bandE, qb==0, i, 1);
         cc6_renormalise_vector(P+C*eBands[i], cc6_Q15ONE, N, C);
         cc6_renormalise_vector(P+C*eBands[i]+1, cc6_Q15ONE, N, C);
         cc6_deinterleave(P+C*eBands[i], C*N);
         for (j=C*eBands[i];j<C*eBands[i+1];j++)
            P[j] = cc6_MULT16_16_Q15(pgains[pband], P[j]);
      } else {
         for (j=C*eBands[i];j<C*eBands[i+1];j++)
            P[j] = 0;
      }
      cc6_deinterleave(X+C*eBands[i], C*N);
      if (q1 > 0)
         cc6_alg_quant(X+C*eBands[i], W+C*eBands[i], N, q1, P+C*eBands[i], enc);
      else
         for (j=C*eBands[i];j<C*eBands[i]+N;j++)
            X[j] = P[j];
      if (q2 > 0)
         cc6_alg_quant(X+C*eBands[i]+N, W+C*eBands[i], N, q2, P+C*eBands[i]+N, enc);
      else
         for (j=C*eBands[i]+N;j<C*eBands[i+1];j++)
            X[j] = 0;

#ifdef FIXED_POINT
      mid = imid;
      side = iside;
#else
      mid = (1./32768)*imid;
      side = (1./32768)*iside;
#endif
      for (c=0;c<C;c++)
         for (j=0;j<N;j++)
            norm[C*(eBands[i]+j)+c] = cc6_MULT16_16_Q15(n,X[C*eBands[i]+c*N+j]);

      for (j=0;j<N;j++)
         X[C*eBands[i]+j] = cc6_MULT16_16_Q15(X[C*eBands[i]+j], mid);
      for (j=0;j<N;j++)
         X[C*eBands[i]+N+j] = cc6_MULT16_16_Q15(X[C*eBands[i]+N+j], side);

      cc6_interleave(X+C*eBands[i], C*N);


      cc6_stereo_band_mix(m, X, bandE, 0, i, -1);
      cc6_renormalise_vector(X+C*eBands[i], cc6_Q15ONE, N, C);
      cc6_renormalise_vector(X+C*eBands[i]+1, cc6_Q15ONE, N, C);
   }
   cc6_RESTORE_STACK;
}
#endif /* DISABLE_STEREO */

/* Decoding of the residual */
void cc6_unquant_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int shortBlocks, int fold, int total_bits, cc6_ec_dec *dec)
{
   int i, j, remaining_bits, balance;
   const cc6_celt_int16_t * __restrict eBands = m->eBands;
   cc6_celt_norm_t * __restrict norm;
   cc6_VARDECL(cc6_celt_norm_t, _norm);
   const cc6_celt_int16_t *pBands = m->pBands;
   int pband=-1;
   int B;
   cc6_SAVE_STACK;

   B = shortBlocks ? m->nbShortMdcts : 1;
   cc6_ALLOC(_norm, eBands[m->nbEBands+1], cc6_celt_norm_t);
   norm = _norm;

   balance = 0;
   for (i=0;i<m->nbEBands;i++)
   {
      int tell;
      int N;
      int q;
      cc6_celt_word16_t n;
      const cc6_celt_int16_t * const *BPbits;
      
      int curr_balance, curr_bits;

      N = eBands[i+1]-eBands[i];
      BPbits = m->bits;

      tell = cc6_ec_dec_tell(dec, 4);
      if (i != 0)
         balance -= tell;
      remaining_bits = (total_bits<<cc6_BITRES)-tell-1;
      curr_balance = (m->nbEBands-i);
      if (curr_balance > 3)
         curr_balance = 3;
      curr_balance = balance / curr_balance;
      q = cc6_bits2pulses(m, BPbits[i], N, pulses[i]+curr_balance);
      curr_bits = cc6_pulses2bits(BPbits[i], N, q);
      remaining_bits -= curr_bits;
      while (remaining_bits < 0 && q > 0)
      {
         remaining_bits += curr_bits;
         q--;
         curr_bits = cc6_pulses2bits(BPbits[i], N, q);
         remaining_bits -= curr_bits;
      }
      balance += pulses[i] + tell;

      n = cc6_SHL16(cc6_celt_sqrt(eBands[i+1]-eBands[i]),11);

      /* If pitch is in use and this eBand begins a pitch band, encode the pitch gain flag */
      if (pitch_used && eBands[i] < m->pitchEnd && eBands[i] == pBands[pband+1])
      {
         int enabled = 1;
         pband++;
         if (remaining_bits >= 1<<cc6_BITRES) {
           enabled = cc6_ec_dec_bits(dec, 1);
           balance += 1<<cc6_BITRES;
         }
         if (enabled)
            pgains[pband] = cc6_QCONST16(.9,15);
         else
            pgains[pband] = 0;
      }

      /* If pitch isn't available, use intra-frame prediction */
      if ((eBands[i] >= m->pitchEnd && fold) || q<=0)
      {
         cc6_intra_fold(m, X+eBands[i], eBands[i+1]-eBands[i], &q, norm, P+eBands[i], eBands[i], B);
      } else if (pitch_used && eBands[i] < m->pitchEnd) {
         for (j=eBands[i];j<eBands[i+1];j++)
            P[j] = cc6_MULT16_16_Q15(pgains[pband], P[j]);
      } else {
         for (j=eBands[i];j<eBands[i+1];j++)
            P[j] = 0;
      }
      
      if (q > 0)
      {
         cc6_alg_unquant(X+eBands[i], eBands[i+1]-eBands[i], q, P+eBands[i], dec);
      } else {
         for (j=eBands[i];j<eBands[i+1];j++)
            X[j] = P[j];
      }
      for (j=eBands[i];j<eBands[i+1];j++)
         norm[j] = cc6_MULT16_16_Q15(n,X[j]);
   }
   cc6_RESTORE_STACK;
}

#ifndef DISABLE_STEREO

void cc6_unquant_bands_stereo(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int shortBlocks, int fold, int total_bits, cc6_ec_dec *dec)
{
   int i, j, remaining_bits, balance;
   const cc6_celt_int16_t * __restrict eBands = m->eBands;
   cc6_celt_norm_t * __restrict norm;
   cc6_VARDECL(cc6_celt_norm_t, _norm);
   const int C = cc6_CHANNELS(m);
   const cc6_celt_int16_t *pBands = m->pBands;
   int pband=-1;
   int B;
   cc6_celt_word16_t mid, side;
   cc6_SAVE_STACK;

   B = shortBlocks ? m->nbShortMdcts : 1;
   cc6_ALLOC(_norm, C*eBands[m->nbEBands+1], cc6_celt_norm_t);
   norm = _norm;

   balance = 0;
   for (i=0;i<m->nbEBands;i++)
   {
      int c;
      int tell;
      int q1, q2;
      cc6_celt_word16_t n;
      const cc6_celt_int16_t * const *BPbits;
      int b, qb;
      int N;
      int curr_balance, curr_bits;
      int imid, iside, itheta;
      int mbits, sbits, delta;
      int qalloc;
      
      BPbits = m->bits;

      N = eBands[i+1]-eBands[i];
      tell = cc6_ec_dec_tell(dec, 4);
      if (i != 0)
         balance -= tell;
      remaining_bits = (total_bits<<cc6_BITRES)-tell-1;
      curr_balance = (m->nbEBands-i);
      if (curr_balance > 3)
         curr_balance = 3;
      curr_balance = balance / curr_balance;
      b = cc6_IMIN(remaining_bits+1,pulses[i]+curr_balance);
      if (b<0)
         b = 0;

      qb = (b-2*(N-1)*(40-cc6_log2_frac(N,4)))/(32*(N-1));
      if (qb > (b>>cc6_BITRES)-1)
         qb = (b>>cc6_BITRES)-1;
      if (qb>14)
         qb = 14;
      if (qb<0)
         qb = 0;
      qalloc = cc6_log2_frac((1<<qb)+1,4);
      if (qb==0)
      {
         itheta=0;
      } else {
         int shift;
         shift = 14-qb;
         itheta = cc6_ec_dec_uint(dec, (1<<qb)+1);
         itheta <<= shift;
      }
      if (itheta == 0)
      {
         imid = 32767;
         iside = 0;
         delta = -10000;
      } else if (itheta == 16384)
      {
         imid = 0;
         iside = 32767;
         delta = 10000;
      } else {
         imid = cc6_bitexact_cos(itheta);
         iside = cc6_bitexact_cos(16384-itheta);
         delta = (N-1)*(cc6_log2_frac(iside,6)-cc6_log2_frac(imid,6))>>2;
      }
      mbits = (b-qalloc/2-delta)/2;
      if (mbits > b-qalloc)
         mbits = b-qalloc;
      if (mbits<0)
         mbits=0;
      sbits = b-qalloc-mbits;
      q1 = cc6_bits2pulses(m, BPbits[i], N, mbits);
      q2 = cc6_bits2pulses(m, BPbits[i], N, sbits);
      curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
      remaining_bits -= curr_bits;
      while (remaining_bits < 0 && (q1 > 0 || q2 > 0))
      {
         remaining_bits += curr_bits;
         if (q1>q2)
         {
            q1--;
            curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
         } else {
            q2--;
            curr_bits = cc6_pulses2bits(BPbits[i], N, q1)+cc6_pulses2bits(BPbits[i], N, q2)+qalloc;
         }
         remaining_bits -= curr_bits;
      }
      balance += pulses[i] + tell;
      
      n = cc6_SHL16(cc6_celt_sqrt((eBands[i+1]-eBands[i])),11);

      /* If pitch is in use and this eBand begins a pitch band, encode the pitch gain flag */
      if (pitch_used && eBands[i]< m->pitchEnd && eBands[i] == pBands[pband+1])
      {
         int enabled = 1;
         pband++;
         if (remaining_bits >= 1<<cc6_BITRES) {
            enabled = cc6_ec_dec_bits(dec, 1);
            balance += 1<<cc6_BITRES;
         }
         if (enabled)
            pgains[pband] = cc6_QCONST16(.9,15);
         else
            pgains[pband] = 0;
      }

      /* If pitch isn't available, use intra-frame prediction */
      if ((eBands[i] >= m->pitchEnd && fold) || (q1+q2)<=0)
      {
         int K[2] = {q1, q2};
         cc6_intra_fold(m, X+C*eBands[i], eBands[i+1]-eBands[i], K, norm, P+C*eBands[i], eBands[i], B);
         cc6_deinterleave(P+C*eBands[i], C*N);
      } else if (pitch_used && eBands[i] < m->pitchEnd) {
         cc6_stereo_band_mix(m, P, bandE, qb==0, i, 1);
         cc6_renormalise_vector(P+C*eBands[i], cc6_Q15ONE, N, C);
         cc6_renormalise_vector(P+C*eBands[i]+1, cc6_Q15ONE, N, C);
         cc6_deinterleave(P+C*eBands[i], C*N);
         for (j=C*eBands[i];j<C*eBands[i+1];j++)
            P[j] = cc6_MULT16_16_Q15(pgains[pband], P[j]);
      } else {
         for (j=C*eBands[i];j<C*eBands[i+1];j++)
            P[j] = 0;
      }
      cc6_deinterleave(X+C*eBands[i], C*N);
      if (q1 > 0)
         cc6_alg_unquant(X+C*eBands[i], N, q1, P+C*eBands[i], dec);
      else
         for (j=C*eBands[i];j<C*eBands[i]+N;j++)
            X[j] = P[j];
      if (q2 > 0)
         cc6_alg_unquant(X+C*eBands[i]+N, N, q2, P+C*eBands[i]+N, dec);
      else
         for (j=C*eBands[i]+N;j<C*eBands[i+1];j++)
            X[j] = 0;
      /*orthogonalize(X+C*eBands[i], X+C*eBands[i]+N, N);*/
      
#ifdef FIXED_POINT
      mid = imid;
      side = iside;
#else
      mid = (1./32768)*imid;
      side = (1./32768)*iside;
#endif
      for (c=0;c<C;c++)
         for (j=0;j<N;j++)
            norm[C*(eBands[i]+j)+c] = cc6_MULT16_16_Q15(n,X[C*eBands[i]+c*N+j]);

      for (j=0;j<N;j++)
         X[C*eBands[i]+j] = cc6_MULT16_16_Q15(X[C*eBands[i]+j], mid);
      for (j=0;j<N;j++)
         X[C*eBands[i]+N+j] = cc6_MULT16_16_Q15(X[C*eBands[i]+N+j], side);

      cc6_interleave(X+C*eBands[i], C*N);

      cc6_stereo_band_mix(m, X, bandE, 0, i, -1);
      cc6_renormalise_vector(X+C*eBands[i], cc6_Q15ONE, N, C);
      cc6_renormalise_vector(X+C*eBands[i]+1, cc6_Q15ONE, N, C);
   }
   cc6_RESTORE_STACK;
}

#endif /* DISABLE_STEREO */
