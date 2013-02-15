/* (C) 2007-2008 Jean-Marc Valin, CSIRO
   (C) 2008 Gregory Maxwell */
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

#define CELT_C

#include "cc6_os_support.h"
#include "cc6_mdct.h"
#include <math.h>
#include "cc6_celt.h"
#include "cc6_pitch.h"
#include "cc6_kiss_fftr.h"
#include "cc6_bands.h"
#include "cc6_modes.h"
#include "cc6_entcode.h"
#include "cc6_quant_bands.h"
#include "cc6_psy.h"
#include "cc6_rate.h"
#include "cc6_stack_alloc.h"
#include "cc6_mathops.h"
#include "cc6_float_cast.h"
#include <stdarg.h>

static const cc6_celt_word16_t cc6_preemph = cc6_QCONST16(0.8f,15);

#ifdef FIXED_POINT
static const cc6_celt_word16_t cc6_transientWindow[16] = {
     279,  1106,  2454,  4276,  6510,  9081, 11900, 14872,
   17896, 20868, 23687, 26258, 28492, 30314, 31662, 32489};
#else
static const float cc6_transientWindow[16] = {
   0.0085135, 0.0337639, 0.0748914, 0.1304955, 
   0.1986827, 0.2771308, 0.3631685, 0.4538658,
   0.5461342, 0.6368315, 0.7228692, 0.8013173, 
   0.8695045, 0.9251086, 0.9662361, 0.9914865};
#endif

#define cc6_ENCODERVALID   0x4c434554
#define cc6_ENCODERPARTIAL 0x5445434c
#define cc6_ENCODERFREED   0x4c004500
   
/** Encoder state 
 @brief Encoder state
 */
struct cc6_CELTEncoder {
   cc6_celt_uint32_t marker;
   const cc6_CELTMode *mode;     /**< Mode used by the encoder */
   int frame_size;
   int block_size;
   int overlap;
   int channels;
   
   int pitch_enabled;       /* Complexity level is allowed to use pitch */
   int pitch_permitted;     /*  Use of the LTP is permitted by the user */
   int pitch_available;     /*  Amount of pitch buffer available */
   int force_intra;
   int delayedIntra;
   cc6_celt_word16_t tonal_average;
   int fold_decision;

   int VBR_rate; /* Target number of 16th bits per frame */
   cc6_celt_word16_t * __restrict preemph_memE;
   cc6_celt_sig_t    * __restrict preemph_memD;

   cc6_celt_sig_t *in_mem;
   cc6_celt_sig_t *out_mem;

   cc6_celt_word16_t *oldBandE;
#ifdef EXP_PSY
   cc6_celt_word16_t *psy_mem;
   struct cc6_PsyDecay psy;
#endif
};

int cc6_check_encoder(const cc6_CELTEncoder *st)
{
   if (st==NULL)
   {
      cc6_celt_warning("NULL passed as an encoder structure");
      return cc6_CELT_INVALID_STATE;
   }
   if (st->marker == cc6_ENCODERVALID)
      return cc6_CELT_OK;
   if (st->marker == cc6_ENCODERFREED)
      cc6_celt_warning("Referencing an encoder that has already been freed");
   else
      cc6_celt_warning("This is not a valid CELT encoder structure");
   return cc6_CELT_INVALID_STATE;
}

cc6_CELTEncoder *cc6_celt_encoder_create(const cc6_CELTMode *mode)
{
   int N, C;
   cc6_CELTEncoder *st;

   if (cc6_check_mode(mode) != cc6_CELT_OK)
      return NULL;

   N = mode->mdctSize;
   C = mode->nbChannels;
   st = cc6_celt_alloc(sizeof(cc6_CELTEncoder));
   
   if (st==NULL) 
      return NULL;   
   st->marker = cc6_ENCODERPARTIAL;
   st->mode = mode;
   st->frame_size = N;
   st->block_size = N;
   st->overlap = mode->overlap;

   st->VBR_rate = 0;
   st->pitch_enabled = 1;
   st->pitch_permitted = 1;
   st->pitch_available = 1;
   st->force_intra  = 0;
   st->delayedIntra = 1;
   st->tonal_average = cc6_QCONST16(1.,8);
   st->fold_decision = 1;

   st->in_mem = cc6_celt_alloc(st->overlap*C*sizeof(cc6_celt_sig_t));
   st->out_mem = cc6_celt_alloc((cc6_MAX_PERIOD+st->overlap)*C*sizeof(cc6_celt_sig_t));

   st->oldBandE = (cc6_celt_word16_t*)cc6_celt_alloc(C*mode->nbEBands*sizeof(cc6_celt_word16_t));

   st->preemph_memE = (cc6_celt_word16_t*)cc6_celt_alloc(C*sizeof(cc6_celt_word16_t));
   st->preemph_memD = (cc6_celt_sig_t*)cc6_celt_alloc(C*sizeof(cc6_celt_sig_t));

#ifdef EXP_PSY
   st->psy_mem = cc6_celt_alloc(cc6_MAX_PERIOD*sizeof(cc6_celt_word16_t));
   cc6_psydecay_init(&st->psy, cc6_MAX_PERIOD/2, st->mode->Fs);
#endif

   if ((st->in_mem!=NULL) && (st->out_mem!=NULL) && (st->oldBandE!=NULL) 
#ifdef EXP_PSY
       && (st->psy_mem!=NULL) 
#endif   
       && (st->preemph_memE!=NULL) && (st->preemph_memD!=NULL))
   {
      st->marker   = cc6_ENCODERVALID;
      return st;
   }
   /* If the setup fails for some reason deallocate it. */
   cc6_celt_encoder_destroy(st);
   return NULL;
}

void cc6_celt_encoder_destroy(cc6_CELTEncoder *st)
{
   if (st == NULL)
   {
      cc6_celt_warning("NULL passed to cc6_celt_encoder_destroy");
      return;
   }

   if (st->marker == cc6_ENCODERFREED)
   {
      cc6_celt_warning("Freeing an encoder which has already been freed");
      return;
   }

   if (st->marker != cc6_ENCODERVALID && st->marker != cc6_ENCODERPARTIAL)
   {
      cc6_celt_warning("This is not a valid CELT encoder structure");
      return;
   }
   /*Check_mode is non-fatal here because we can still free
    the encoder memory even if the mode is bad, although calling
    the free functions in this order is a violation of the API.*/
   cc6_check_mode(st->mode);
   
   cc6_celt_free(st->in_mem);
   cc6_celt_free(st->out_mem);
   
   cc6_celt_free(st->oldBandE);
   
   cc6_celt_free(st->preemph_memE);
   cc6_celt_free(st->preemph_memD);
   
#ifdef EXP_PSY
   cc6_celt_free (st->psy_mem);
   cc6_psydecay_clear(&st->psy);
#endif
   st->marker = cc6_ENCODERFREED;
   
   cc6_celt_free(st);
}

static __inline cc6_celt_int16_t cc6_FLOAT2INT16(float x)
{
   x = x*cc6_CELT_SIG_SCALE;
   x = cc6_MAX32(x, -32768);
   x = cc6_MIN32(x, 32767);
   return (cc6_celt_int16_t)cc6_float2int(x);
}

static __inline cc6_celt_word16_t cc6_SIG2WORD16(cc6_celt_sig_t x)
{
#ifdef FIXED_POINT
   x = cc6_PSHR32(x, cc6_SIG_SHIFT);
   x = cc6_MAX32(x, -32768);
   x = cc6_MIN32(x, 32767);
   return cc6_EXTRACT16(x);
#else
   return (cc6_celt_word16_t)x;
#endif
}

static int cc6_transient_analysis(cc6_celt_word32_t *in, int len, int C, int *transient_time, int *transient_shift)
{
   int c, i, n;
   cc6_celt_word32_t ratio;
   cc6_VARDECL(cc6_celt_word32_t, begin);
   cc6_SAVE_STACK;
   cc6_ALLOC(begin, len, cc6_celt_word32_t);
   for (i=0;i<len;i++)
      begin[i] = cc6_ABS32(cc6_SHR32(in[C*i],cc6_SIG_SHIFT));
   for (c=1;c<C;c++)
   {
      for (i=0;i<len;i++)
         begin[i] = cc6_MAX32(begin[i], cc6_ABS32(cc6_SHR32(in[C*i+c],cc6_SIG_SHIFT)));
   }
   for (i=1;i<len;i++)
      begin[i] = cc6_MAX32(begin[i-1],begin[i]);
   n = -1;
   for (i=8;i<len-8;i++)
   {
      if (begin[i] < cc6_MULT16_32_Q15(cc6_QCONST16(.2f,15),begin[len-1]))
         n=i;
   }
   if (n<32)
   {
      n = -1;
      ratio = 0;
   } else {
      ratio = cc6_DIV32(begin[len-1],1+begin[n-16]);
   }
   if (ratio < 0)
      ratio = 0;
   if (ratio > 1000)
      ratio = 1000;
   ratio *= ratio;
   
   if (ratio > 2048)
      *transient_shift = 3;
   else
      *transient_shift = 0;
   
   *transient_time = n;
   
   cc6_RESTORE_STACK;
   return ratio > 20;
}

/** Apply window and compute the MDCT for all sub-frames and 
    all channels in a frame */
static void cc6_compute_mdcts(const cc6_CELTMode *mode, int shortBlocks, cc6_celt_sig_t * __restrict in, cc6_celt_sig_t * __restrict out)
{
   const int C = cc6_CHANNELS(mode);
   if (C==1 && !shortBlocks)
   {
      const cc6_mdct_lookup *lookup = cc6_MDCT(mode);
      const int overlap = cc6_OVERLAP(mode);
      cc6_mdct_forward(lookup, in, out, mode->window, overlap);
   } else if (!shortBlocks) {
      const cc6_mdct_lookup *lookup = cc6_MDCT(mode);
      const int overlap = cc6_OVERLAP(mode);
      const int N = cc6_FRAMESIZE(mode);
      int c;
      cc6_VARDECL(cc6_celt_word32_t, x);
      cc6_VARDECL(cc6_celt_word32_t, tmp);
      cc6_SAVE_STACK;
      cc6_ALLOC(x, N+overlap, cc6_celt_word32_t);
      cc6_ALLOC(tmp, N, cc6_celt_word32_t);
      for (c=0;c<C;c++)
      {
         int j;
         for (j=0;j<N+overlap;j++)
            x[j] = in[C*j+c];
         cc6_mdct_forward(lookup, x, tmp, mode->window, overlap);
         /* Interleaving the sub-frames */
         for (j=0;j<N;j++)
            out[j+c*N] = tmp[j];
      }
      cc6_RESTORE_STACK;
   } else {
      const cc6_mdct_lookup *lookup = &mode->shortMdct;
      const int overlap = mode->overlap;
      const int N = mode->shortMdctSize;
      int b, c;
      cc6_VARDECL(cc6_celt_word32_t, x);
      cc6_VARDECL(cc6_celt_word32_t, tmp);
      cc6_SAVE_STACK;
      cc6_ALLOC(x, N+overlap, cc6_celt_word32_t);
      cc6_ALLOC(tmp, N, cc6_celt_word32_t);
      for (c=0;c<C;c++)
      {
         int B = mode->nbShortMdcts;
         for (b=0;b<B;b++)
         {
            int j;
            for (j=0;j<N+overlap;j++)
               x[j] = in[C*(b*N+j)+c];
            cc6_mdct_forward(lookup, x, tmp, mode->window, overlap);
            /* Interleaving the sub-frames */
            for (j=0;j<N;j++)
               out[(j*B+b)+c*N*B] = tmp[j];
         }
      }
      cc6_RESTORE_STACK;
   }
}

/** Compute the IMDCT and apply window for all sub-frames and 
    all channels in a frame */
static void cc6_compute_inv_mdcts(const cc6_CELTMode *mode, int shortBlocks, cc6_celt_sig_t *X, int transient_time, int transient_shift, cc6_celt_sig_t * __restrict out_mem)
{
   int c, N4;
   const int C = cc6_CHANNELS(mode);
   const int N = cc6_FRAMESIZE(mode);
   const int overlap = cc6_OVERLAP(mode);
   N4 = (N-overlap)>>1;
   for (c=0;c<C;c++)
   {
      int j;
      if (transient_shift==0 && C==1 && !shortBlocks) {
         const cc6_mdct_lookup *lookup = cc6_MDCT(mode);
         cc6_mdct_backward(lookup, X, out_mem+C*(cc6_MAX_PERIOD-N-N4), mode->window, overlap);
      } else if (!shortBlocks) {
         const cc6_mdct_lookup *lookup = cc6_MDCT(mode);
         cc6_VARDECL(cc6_celt_word32_t, x);
         cc6_VARDECL(cc6_celt_word32_t, tmp);
         cc6_SAVE_STACK;
         cc6_ALLOC(x, 2*N, cc6_celt_word32_t);
         cc6_ALLOC(tmp, N, cc6_celt_word32_t);
         /* De-interleaving the sub-frames */
         for (j=0;j<N;j++)
            tmp[j] = X[j+c*N];
         /* Prevents problems from the imdct doing the overlap-add */
         cc6_CELT_MEMSET(x+N4, 0, N);
         cc6_mdct_backward(lookup, tmp, x, mode->window, overlap);
         cc6_celt_assert(transient_shift == 0);
         /* The first and last part would need to be set to zero if we actually
            wanted to use them. */
         for (j=0;j<overlap;j++)
            out_mem[C*(cc6_MAX_PERIOD-N)+C*j+c] += x[j+N4];
         for (j=0;j<overlap;j++)
            out_mem[C*(cc6_MAX_PERIOD)+C*(overlap-j-1)+c] = x[2*N-j-N4-1];
         for (j=0;j<2*N4;j++)
            out_mem[C*(cc6_MAX_PERIOD-N)+C*(j+overlap)+c] = x[j+N4+overlap];
         cc6_RESTORE_STACK;
      } else {
         int b;
         const int N2 = mode->shortMdctSize;
         const int B = mode->nbShortMdcts;
         const cc6_mdct_lookup *lookup = &mode->shortMdct;
         cc6_VARDECL(cc6_celt_word32_t, x);
         cc6_VARDECL(cc6_celt_word32_t, tmp);
         cc6_SAVE_STACK;
         cc6_ALLOC(x, 2*N, cc6_celt_word32_t);
         cc6_ALLOC(tmp, N, cc6_celt_word32_t);
         /* Prevents problems from the imdct doing the overlap-add */
         cc6_CELT_MEMSET(x+N4, 0, N2);
         for (b=0;b<B;b++)
         {
            /* De-interleaving the sub-frames */
            for (j=0;j<N2;j++)
               tmp[j] = X[(j*B+b)+c*N2*B];
            cc6_mdct_backward(lookup, tmp, x+N4+N2*b, mode->window, overlap);
         }
         if (transient_shift > 0)
         {
#ifdef FIXED_POINT
            for (j=0;j<16;j++)
               x[N4+transient_time+j-16] = cc6_MULT16_32_Q15(cc6_SHR16(cc6_Q15_ONE-cc6_transientWindow[j],transient_shift)+cc6_transientWindow[j], cc6_SHL32(x[N4+transient_time+j-16],transient_shift));
            for (j=transient_time;j<N+overlap;j++)
               x[N4+j] = cc6_SHL32(x[N4+j], transient_shift);
#else
            for (j=0;j<16;j++)
               x[N4+transient_time+j-16] *= 1+cc6_transientWindow[j]*((1<<transient_shift)-1);
            for (j=transient_time;j<N+overlap;j++)
               x[N4+j] *= 1<<transient_shift;
#endif
         }
         /* The first and last part would need to be set to zero 
            if we actually wanted to use them. */
         for (j=0;j<overlap;j++)
            out_mem[C*(cc6_MAX_PERIOD-N)+C*j+c] += x[j+N4];
         for (j=0;j<overlap;j++)
            out_mem[C*(cc6_MAX_PERIOD)+C*(overlap-j-1)+c] = x[2*N-j-N4-1];
         for (j=0;j<2*N4;j++)
            out_mem[C*(cc6_MAX_PERIOD-N)+C*(j+overlap)+c] = x[j+N4+overlap];
         cc6_RESTORE_STACK;
      }
   }
}

#define cc6_FLAG_NONE        0
#define cc6_FLAG_INTRA       1U<<16
#define cc6_FLAG_PITCH       1U<<15
#define cc6_FLAG_SHORT       1U<<14
#define cc6_FLAG_FOLD        1U<<13
#define cc6_FLAG_MASK        (cc6_FLAG_INTRA|cc6_FLAG_PITCH|cc6_FLAG_SHORT|cc6_FLAG_FOLD)

cc6_celt_int32_t cc6_flaglist[8] = {
      0 /*00  */ | cc6_FLAG_FOLD,
      1 /*01  */ | cc6_FLAG_PITCH|cc6_FLAG_FOLD,
      8 /*1000*/ | cc6_FLAG_NONE,
      9 /*1001*/ | cc6_FLAG_SHORT|cc6_FLAG_FOLD,
     10 /*1010*/ | cc6_FLAG_PITCH,
     11 /*1011*/ | cc6_FLAG_INTRA,
      6 /*110 */ | cc6_FLAG_INTRA|cc6_FLAG_FOLD,
      7 /*111 */ | cc6_FLAG_INTRA|cc6_FLAG_SHORT|cc6_FLAG_FOLD
};

void cc6_encode_flags(cc6_ec_enc *enc, int intra_ener, int has_pitch, int shortBlocks, int has_fold)
{
   int i;
   int flags=cc6_FLAG_NONE;
   int flag_bits;
   flags |= intra_ener   ? cc6_FLAG_INTRA : 0;
   flags |= has_pitch    ? cc6_FLAG_PITCH : 0;
   flags |= shortBlocks  ? cc6_FLAG_SHORT : 0;
   flags |= has_fold     ? cc6_FLAG_FOLD  : 0;
   for (i=0;i<8;i++)
      if (flags == (cc6_flaglist[i]&cc6_FLAG_MASK))
         break;
   cc6_celt_assert(i<8);
   flag_bits = cc6_flaglist[i]&0xf;
   /*printf ("enc %d: %d %d %d %d\n", flag_bits, intra_ener, has_pitch, shortBlocks, has_fold);*/
   if (i<2)
      cc6_ec_enc_bits(enc, flag_bits, 2);
   else if (i<6)
      cc6_ec_enc_bits(enc, flag_bits, 4);
   else
      cc6_ec_enc_bits(enc, flag_bits, 3);
}

void cc6_decode_flags(cc6_ec_dec *dec, int *intra_ener, int *has_pitch, int *shortBlocks, int *has_fold)
{
   int i;
   int flag_bits;
   flag_bits = cc6_ec_dec_bits(dec, 2);
   /*printf ("(%d) ", flag_bits);*/
   if (flag_bits==2)
      flag_bits = (flag_bits<<2) | cc6_ec_dec_bits(dec, 2);
   else if (flag_bits==3)
      flag_bits = (flag_bits<<1) | cc6_ec_dec_bits(dec, 1);
   for (i=0;i<8;i++)
      if (flag_bits == (cc6_flaglist[i]&0xf))
         break;
   cc6_celt_assert(i<8);
   *intra_ener  = (cc6_flaglist[i]&cc6_FLAG_INTRA) != 0;
   *has_pitch   = (cc6_flaglist[i]&cc6_FLAG_PITCH) != 0;
   *shortBlocks = (cc6_flaglist[i]&cc6_FLAG_SHORT) != 0;
   *has_fold    = (cc6_flaglist[i]&cc6_FLAG_FOLD ) != 0;
   /*printf ("dec %d: %d %d %d %d\n", flag_bits, *intra_ener, *has_pitch, *shortBlocks, *has_fold);*/
}

#ifdef FIXED_POINT
int cc6_celt_encode(cc6_CELTEncoder * __restrict st, const cc6_celt_int16_t * pcm, cc6_celt_int16_t * optional_synthesis, unsigned char *compressed, int nbCompressedBytes)
{
#else
int cc6_celt_encode_float(cc6_CELTEncoder * __restrict st, const cc6_celt_sig_t * pcm, cc6_celt_sig_t * optional_synthesis, unsigned char *compressed, int nbCompressedBytes)
{
#endif
   int i, c, N, N4;
   int has_pitch;
   int pitch_index;
   int bits;
   int has_fold=1;
   unsigned coarse_needed;
   cc6_ec_byte_buffer buf;
   cc6_ec_enc         enc;
   cc6_VARDECL(cc6_celt_sig_t, in);
   cc6_VARDECL(cc6_celt_sig_t, freq);
   cc6_VARDECL(cc6_celt_norm_t, X);
   cc6_VARDECL(cc6_celt_norm_t, P);
   cc6_VARDECL(cc6_celt_ener_t, bandE);
   cc6_VARDECL(cc6_celt_word16_t, bandLogE);
   cc6_VARDECL(cc6_celt_pgain_t, gains);
   cc6_VARDECL(int, fine_quant);
   cc6_VARDECL(cc6_celt_word16_t, error);
   cc6_VARDECL(int, pulses);
   cc6_VARDECL(int, offsets);
   cc6_VARDECL(int, fine_priority);
#ifdef EXP_PSY
   cc6_VARDECL(cc6_celt_word32_t, mask);
   cc6_VARDECL(cc6_celt_word32_t, tonality);
   cc6_VARDECL(cc6_celt_word32_t, bandM);
   cc6_VARDECL(cc6_celt_ener_t, bandN);
#endif
   int intra_ener = 0;
   int shortBlocks=0;
   int transient_time;
   int transient_shift;
   const int C = cc6_CHANNELS(st->mode);
   int mdct_weight_shift = 0;
   int mdct_weight_pos=0;
   cc6_SAVE_STACK;

   if (cc6_check_encoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (nbCompressedBytes<0 || pcm==NULL)
     return cc6_CELT_BAD_ARG;

   /* The memset is important for now in case the encoder doesn't 
      fill up all the bytes */
   cc6_CELT_MEMSET(compressed, 0, nbCompressedBytes);
   cc6_ec_byte_writeinit_buffer(&buf, compressed, nbCompressedBytes);
   cc6_ec_enc_init(&enc,&buf);

   N = st->block_size;
   N4 = (N-st->overlap)>>1;
   cc6_ALLOC(in, 2*C*N-2*C*N4, cc6_celt_sig_t);

   cc6_CELT_COPY(in, st->in_mem, C*st->overlap);
   for (c=0;c<C;c++)
   {
      const cc6_celt_word16_t * __restrict pcmp = pcm+c;
      cc6_celt_sig_t * __restrict inp = in+C*st->overlap+c;
      for (i=0;i<N;i++)
      {
         /* Apply pre-emphasis */
         cc6_celt_sig_t tmp = cc6_SCALEIN(cc6_SHL32(cc6_EXTEND32(*pcmp), cc6_SIG_SHIFT));
         *inp = cc6_SUB32(tmp, cc6_SHR32(cc6_MULT16_16(cc6_preemph,st->preemph_memE[c]),3));
         st->preemph_memE[c] = cc6_SCALEIN(*pcmp);
         inp += C;
         pcmp += C;
      }
   }
   cc6_CELT_COPY(st->in_mem, in+C*(2*N-2*N4-st->overlap), C*st->overlap);

   /* Transient handling */
   transient_time = -1;
   transient_shift = 0;
   shortBlocks = 0;

   if (st->mode->nbShortMdcts > 1 && cc6_transient_analysis(in, N+st->overlap, C, &transient_time, &transient_shift))
   {
#ifndef FIXED_POINT
      float gain_1;
#endif
      /* Apply the inverse shaping window */
      if (transient_shift)
      {
#ifdef FIXED_POINT
         for (c=0;c<C;c++)
            for (i=0;i<16;i++)
               in[C*(transient_time+i-16)+c] = cc6_MULT16_32_Q15(cc6_EXTRACT16(cc6_SHR32(cc6_celt_rcp(cc6_Q15ONE+cc6_MULT16_16(cc6_transientWindow[i],((1<<transient_shift)-1))),1)), in[C*(transient_time+i-16)+c]);
         for (c=0;c<C;c++)
            for (i=transient_time;i<N+st->overlap;i++)
               in[C*i+c] = cc6_SHR32(in[C*i+c], transient_shift);
#else
         for (c=0;c<C;c++)
            for (i=0;i<16;i++)
               in[C*(transient_time+i-16)+c] /= 1+cc6_transientWindow[i]*((1<<transient_shift)-1);
         gain_1 = 1./(1<<transient_shift);
         for (c=0;c<C;c++)
            for (i=transient_time;i<N+st->overlap;i++)
               in[C*i+c] *= gain_1;
#endif
      }
      shortBlocks = 1;
      has_fold = 1;
   }

   cc6_ALLOC(freq, C*N, cc6_celt_sig_t); /**< Interleaved signal MDCTs */
   cc6_ALLOC(bandE,st->mode->nbEBands*C, cc6_celt_ener_t);
   cc6_ALLOC(bandLogE,st->mode->nbEBands*C, cc6_celt_word16_t);
   /* Compute MDCTs */
   cc6_compute_mdcts(st->mode, shortBlocks, in, freq);

   if (shortBlocks && !transient_shift) 
   {
      cc6_celt_word32_t sum[8]={1,1,1,1,1,1,1,1};
      int m;
      for (c=0;c<C;c++)
      {
         m=0;
         do {
            cc6_celt_word32_t tmp=0;
            for (i=m+c*N;i<(c+1)*N;i+=st->mode->nbShortMdcts)
               tmp += cc6_ABS32(freq[i]);
            sum[m++] += tmp;
         } while (m<st->mode->nbShortMdcts);
      }
      m=0;
#ifdef FIXED_POINT
      do {
         if (cc6_SHR32(sum[m+1],3) > sum[m])
         {
            mdct_weight_shift=2;
            mdct_weight_pos = m;
         } else if (cc6_SHR32(sum[m+1],1) > sum[m] && mdct_weight_shift < 2)
         {
            mdct_weight_shift=1;
            mdct_weight_pos = m;
         }
         m++;
      } while (m<st->mode->nbShortMdcts-1);
      if (mdct_weight_shift)
      {
         for (c=0;c<C;c++)
            for (m=mdct_weight_pos+1;m<st->mode->nbShortMdcts;m++)
               for (i=m+c*N;i<(c+1)*N;i+=st->mode->nbShortMdcts)
                  freq[i] = cc6_SHR32(freq[i],mdct_weight_shift);
      }
#else
      do {
         if (sum[m+1] > 8*sum[m])
         {
            mdct_weight_shift=2;
            mdct_weight_pos = m;
         } else if (sum[m+1] > 2*sum[m] && mdct_weight_shift < 2)
         {
            mdct_weight_shift=1;
            mdct_weight_pos = m;
         }
         m++;
      } while (m<st->mode->nbShortMdcts-1);
      if (mdct_weight_shift)
      {
         for (c=0;c<C;c++)
            for (m=mdct_weight_pos+1;m<st->mode->nbShortMdcts;m++)
               for (i=m+c*N;i<(c+1)*N;i+=st->mode->nbShortMdcts)
                  freq[i] = (1./(1<<mdct_weight_shift))*freq[i];
      }
#endif
   }

   cc6_compute_band_energies(st->mode, freq, bandE);
   for (i=0;i<st->mode->nbEBands*C;i++)
      bandLogE[i] = amp2Log(bandE[i]);

   /* Don't use intra energy when we're operating at low bit-rate */
   intra_ener = st->force_intra || (st->delayedIntra && nbCompressedBytes > st->mode->nbEBands);
   if (shortBlocks || cc6_intra_decision(bandLogE, st->oldBandE, st->mode->nbEBands))
      st->delayedIntra = 1;
   else
      st->delayedIntra = 0;

   /* Pitch analysis: we do it early to save on the peak stack space */
   /* Don't use pitch if there isn't enough data available yet, 
      or if we're using shortBlocks */
   has_pitch = st->pitch_enabled && st->pitch_permitted && (N <= 512) 
            && (st->pitch_available >= cc6_MAX_PERIOD) && (!shortBlocks)
            && !intra_ener;
#ifdef EXP_PSY
   cc6_ALLOC(tonality, cc6_MAX_PERIOD/4, cc6_celt_word16_t);
   {
      cc6_VARDECL(cc6_celt_word16_t, X);
      cc6_ALLOC(X, cc6_MAX_PERIOD/2, cc6_celt_word16_t);
      cc6_find_spectral_pitch(st->mode, st->mode->fft, &st->mode->psy, in, st->out_mem, st->mode->window, X, 2*N-2*N4, cc6_MAX_PERIOD-(2*N-2*N4), &pitch_index);
      cc6_compute_tonality(st->mode, X, st->psy_mem, cc6_MAX_PERIOD, tonality, cc6_MAX_PERIOD/4);
   }
#else
   if (has_pitch)
   {
      cc6_find_spectral_pitch(st->mode, st->mode->fft, &st->mode->psy, in, st->out_mem, st->mode->window, NULL, 2*N-2*N4, cc6_MAX_PERIOD-(2*N-2*N4), &pitch_index);
   }
#endif

#ifdef EXP_PSY
   cc6_ALLOC(mask, N, cc6_celt_sig_t);
   cc6_compute_mdct_masking(&st->psy, freq, tonality, st->psy_mem, mask, C*N);
   /*for (i=0;i<256;i++)
      printf ("%f %f %f ", freq[i], tonality[i], mask[i]);
   printf ("\n");*/
#endif

   /* Deferred allocation after cc6_find_spectral_pitch() to reduce
      the peak memory usage */
   cc6_ALLOC(X, C*N, cc6_celt_norm_t);         /**< Interleaved normalised MDCTs */
   cc6_ALLOC(P, C*N, cc6_celt_norm_t);         /**< Interleaved normalised pitch MDCTs*/
   cc6_ALLOC(gains,st->mode->nbPBands, cc6_celt_pgain_t);


   /* Band normalisation */
   cc6_normalise_bands(st->mode, freq, X, bandE);
   if (!shortBlocks && !cc6_folding_decision(st->mode, X, &st->tonal_average, &st->fold_decision))
      has_fold = 0;
#ifdef EXP_PSY
   cc6_ALLOC(bandN,C*st->mode->nbEBands, cc6_celt_ener_t);
   cc6_ALLOC(bandM,st->mode->nbEBands, cc6_celt_ener_t);
   cc6_compute_noise_energies(st->mode, freq, tonality, bandN);

   /*for (i=0;i<st->mode->nbEBands;i++)
      printf ("%f ", (.1+bandN[i])/(.1+bandE[i]));
   printf ("\n");*/
   has_fold = 0;
   for (i=st->mode->nbPBands;i<st->mode->nbEBands;i++)
      if (bandN[i] < .4*bandE[i])
         has_fold++;
   /*printf ("%d\n", has_fold);*/
   if (has_fold>=2)
      has_fold = 0;
   else
      has_fold = 1;
   for (i=0;i<N;i++)
      mask[i] = sqrt(mask[i]);
   cc6_compute_band_energies(st->mode, mask, bandM);
   /*for (i=0;i<st->mode->nbEBands;i++)
      printf ("%f %f ", bandE[i], bandM[i]);
   printf ("\n");*/
#endif

   /* Compute MDCTs of the pitch part */
   if (has_pitch)
   {
      cc6_celt_word32_t curr_power, pitch_power=0;
      /* Normalise the pitch vector as well (discard the energies) */
      cc6_VARDECL(cc6_celt_ener_t, bandEp);
      
      cc6_compute_mdcts(st->mode, 0, st->out_mem+pitch_index*C, freq);
      cc6_ALLOC(bandEp, st->mode->nbEBands*st->mode->nbChannels, cc6_celt_ener_t);
      cc6_compute_band_energies(st->mode, freq, bandEp);
      cc6_normalise_bands(st->mode, freq, P, bandEp);
      pitch_power = bandEp[0]+bandEp[1]+bandEp[2];
      curr_power = bandE[0]+bandE[1]+bandE[2];
      if (C>1)
      {
         pitch_power += bandEp[0+st->mode->nbEBands]+bandEp[1+st->mode->nbEBands]+bandEp[2+st->mode->nbEBands];
         curr_power += bandE[0+st->mode->nbEBands]+bandE[1+st->mode->nbEBands]+bandE[2+st->mode->nbEBands];
      }
      /* Check if we can safely use the pitch (i.e. effective gain 
      isn't too high) */
      if ((cc6_MULT16_32_Q15(cc6_QCONST16(.1f, 15),curr_power) + cc6_QCONST32(10.f,cc6_ENER_SHIFT) < pitch_power))
      {
         /* Pitch prediction */
         has_pitch = cc6_compute_pitch_gain(st->mode, X, P, gains);
      } else {
         has_pitch = 0;
      }
   }
   
   cc6_encode_flags(&enc, intra_ener, has_pitch, shortBlocks, has_fold);
   if (has_pitch)
   {
      cc6_ec_enc_uint(&enc, pitch_index, cc6_MAX_PERIOD-(2*N-2*N4));
   } else {
      for (i=0;i<st->mode->nbPBands;i++)
         gains[i] = 0;
      for (i=0;i<C*N;i++)
         P[i] = 0;
   }
   if (shortBlocks)
   {
      if (transient_shift)
      {
         cc6_ec_enc_bits(&enc, transient_shift, 2);
         cc6_ec_enc_uint(&enc, transient_time, N+st->overlap);
      } else {
         cc6_ec_enc_bits(&enc, mdct_weight_shift, 2);
         if (mdct_weight_shift && st->mode->nbShortMdcts!=2)
            cc6_ec_enc_uint(&enc, mdct_weight_pos, st->mode->nbShortMdcts-1);
      }
   }

#ifdef STDIN_TUNING2
   static int fine_quant[30];
   static int pulses[30];
   static int init=0;
   if (!init)
   {
      for (i=0;i<st->mode->nbEBands;i++)
         scanf("%d ", &fine_quant[i]);
      for (i=0;i<st->mode->nbEBands;i++)
         scanf("%d ", &pulses[i]);
      init = 1;
   }
#else
   cc6_ALLOC(fine_quant, st->mode->nbEBands, int);
   cc6_ALLOC(pulses, st->mode->nbEBands, int);
#endif

   /* Bit allocation */
   cc6_ALLOC(error, C*st->mode->nbEBands, cc6_celt_word16_t);
   coarse_needed = cc6_quant_coarse_energy(st->mode, bandLogE, st->oldBandE, nbCompressedBytes*8/3, intra_ener, st->mode->prob, error, &enc);
   coarse_needed = ((coarse_needed*3-1)>>3)+1;

   /* Variable bitrate */
   if (st->VBR_rate>0)
   {
     /* The target rate in 16th bits per frame */
     int target=st->VBR_rate;
   
     /* Shortblocks get a large boost in bitrate, but since they 
        are uncommon long blocks are not greatly effected */
     if (shortBlocks)
       target*=2;
     else if (st->mode->nbShortMdcts > 1)
       target-=(target+14)/28;     

     /* The average energy is removed from the target and the actual 
        energy added*/
     target=target-588+cc6_ec_enc_tell(&enc, 4);

     /* In VBR mode the frame size must not be reduced so much that it would result in the coarse energy busting its budget */
     target=cc6_IMAX(coarse_needed,(target+64)/128);
     nbCompressedBytes=cc6_IMIN(nbCompressedBytes,target);
   }

   cc6_ALLOC(offsets, st->mode->nbEBands, int);
   cc6_ALLOC(fine_priority, st->mode->nbEBands, int);

   for (i=0;i<st->mode->nbEBands;i++)
      offsets[i] = 0;
   bits = nbCompressedBytes*8 - cc6_ec_enc_tell(&enc, 0) - 1;
   if (has_pitch)
      bits -= st->mode->nbPBands;
#ifndef STDIN_TUNING
   cc6_compute_allocation(st->mode, offsets, bits, pulses, fine_quant, fine_priority);
#endif

   cc6_quant_fine_energy(st->mode, bandE, st->oldBandE, error, fine_quant, &enc);

   /* Residual quantisation */
   if (C==1)
      cc6_quant_bands(st->mode, X, P, NULL, has_pitch, gains, bandE, pulses, shortBlocks, has_fold, nbCompressedBytes*8, &enc);
#ifndef DISABLE_STEREO
   else
      cc6_quant_bands_stereo(st->mode, X, P, NULL, has_pitch, gains, bandE, pulses, shortBlocks, has_fold, nbCompressedBytes*8, &enc);
#endif

   cc6_quant_energy_finalise(st->mode, bandE, st->oldBandE, error, fine_quant, fine_priority, nbCompressedBytes*8-cc6_ec_enc_tell(&enc, 0), &enc);

   /* Re-synthesis of the coded audio if required */
   if (st->pitch_available>0 || optional_synthesis!=NULL)
   {
      if (st->pitch_available>0 && st->pitch_available<cc6_MAX_PERIOD)
        st->pitch_available+=st->frame_size;

      /* Synthesis */
      cc6_denormalise_bands(st->mode, X, freq, bandE);
      
      
      cc6_CELT_MOVE(st->out_mem, st->out_mem+C*N, C*(cc6_MAX_PERIOD+st->overlap-N));
      
      if (mdct_weight_shift)
      {
         int m;
         for (c=0;c<C;c++)
            for (m=mdct_weight_pos+1;m<st->mode->nbShortMdcts;m++)
               for (i=m+c*N;i<(c+1)*N;i+=st->mode->nbShortMdcts)
#ifdef FIXED_POINT
                  freq[i] = cc6_SHL32(freq[i], mdct_weight_shift);
#else
                  freq[i] = (1<<mdct_weight_shift)*freq[i];
#endif
      }
      cc6_compute_inv_mdcts(st->mode, shortBlocks, freq, transient_time, transient_shift, st->out_mem);
      /* De-emphasis and put everything back at the right place 
         in the synthesis history */
      if (optional_synthesis != NULL) {
         for (c=0;c<C;c++)
         {
            int j;
            for (j=0;j<N;j++)
            {
               cc6_celt_sig_t tmp = cc6_MAC16_32_Q15(st->out_mem[C*(cc6_MAX_PERIOD-N)+C*j+c],
                                   cc6_preemph,st->preemph_memD[c]);
               st->preemph_memD[c] = tmp;
               optional_synthesis[C*j+c] = cc6_SCALEOUT(cc6_SIG2WORD16(tmp));
            }
         }
      }
   }

   cc6_ec_enc_done(&enc);
   
   cc6_RESTORE_STACK;
   return nbCompressedBytes;
}

#ifdef FIXED_POINT
#ifndef DISABLE_FLOAT_API
int cc6_celt_encode_float(cc6_CELTEncoder * __restrict st, const float * pcm, float * optional_synthesis, unsigned char *compressed, int nbCompressedBytes)
{
   int j, ret, C, N;
   cc6_VARDECL(cc6_celt_int16_t, in);

   if (cc6_check_encoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (pcm==NULL)
      return cc6_CELT_BAD_ARG;

   cc6_SAVE_STACK;
   C = cc6_CHANNELS(st->mode);
   N = st->block_size;
   cc6_ALLOC(in, C*N, cc6_celt_int16_t);

   for (j=0;j<C*N;j++)
     in[j] = cc6_FLOAT2INT16(pcm[j]);

   if (optional_synthesis != NULL) {
     ret=cc6_celt_encode(st,in,in,compressed,nbCompressedBytes);
      for (j=0;j<C*N;j++)
         optional_synthesis[j]=in[j]*(1/32768.);
   } else {
     ret=cc6_celt_encode(st,in,NULL,compressed,nbCompressedBytes);
   }
   cc6_RESTORE_STACK;
   return ret;

}
#endif /*DISABLE_FLOAT_API*/
#else
int cc6_celt_encode(cc6_CELTEncoder * __restrict st, const cc6_celt_int16_t * pcm, cc6_celt_int16_t * optional_synthesis, unsigned char *compressed, int nbCompressedBytes)
{
   int j, ret, C, N;
   cc6_VARDECL(cc6_celt_sig_t, in);

   if (cc6_check_encoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (pcm==NULL)
      return cc6_CELT_BAD_ARG;

   cc6_SAVE_STACK;
   C=cc6_CHANNELS(st->mode);
   N=st->block_size;
   cc6_ALLOC(in, C*N, cc6_celt_sig_t);
   for (j=0;j<C*N;j++) {
     in[j] = cc6_SCALEOUT(pcm[j]);
   }

   if (optional_synthesis != NULL) {
      ret = cc6_celt_encode_float(st,in,in,compressed,nbCompressedBytes);
      for (j=0;j<C*N;j++)
         optional_synthesis[j] = cc6_FLOAT2INT16(in[j]);
   } else {
      ret = cc6_celt_encode_float(st,in,NULL,compressed,nbCompressedBytes);
   }
   cc6_RESTORE_STACK;
   return ret;
}
#endif

int cc6_celt_encoder_ctl(cc6_CELTEncoder * __restrict st, int request, ...)
{
   va_list ap;
   
   if (cc6_check_encoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   va_start(ap, request);
   if ((request!=cc6_CELT_GET_MODE_REQUEST) && (cc6_check_mode(st->mode) != cc6_CELT_OK))
     goto bad_mode;
   switch (request)
   {
      case cc6_CELT_GET_MODE_REQUEST:
      {
         const cc6_CELTMode ** value = va_arg(ap, const cc6_CELTMode**);
         if (value==0)
            goto bad_arg;
         *value=st->mode;
      }
      break;
      case cc6_CELT_SET_COMPLEXITY_REQUEST:
      {
         int value = va_arg(ap, cc6_celt_int32_t);
         if (value<0 || value>10)
            goto bad_arg;
         if (value<=2) {
            st->pitch_enabled = 0; 
            st->pitch_available = 0;
         } else {
              st->pitch_enabled = 1;
              if (st->pitch_available<1)
                st->pitch_available = 1;
         }   
      }
      break;
      case cc6_CELT_SET_PREDICTION_REQUEST:
      {
         int value = va_arg(ap, cc6_celt_int32_t);
         if (value<0 || value>2)
            goto bad_arg;
         if (value==0)
         {
            st->force_intra   = 1;
            st->pitch_permitted = 0;
         } else if (value==1) {
            st->force_intra   = 0;
            st->pitch_permitted = 0;
         } else {
            st->force_intra   = 0;
            st->pitch_permitted = 1;
         }   
      }
      break;
      case cc6_CELT_SET_VBR_RATE_REQUEST:
      {
         int value = va_arg(ap, cc6_celt_int32_t);
         if (value<0)
            goto bad_arg;
         if (value>3072000)
            value = 3072000;
         st->VBR_rate = ((st->mode->Fs<<3)+(st->block_size>>1))/st->block_size;
         st->VBR_rate = ((value<<7)+(st->VBR_rate>>1))/st->VBR_rate;
      }
      break;
      case cc6_CELT_RESET_STATE:
      {
         const cc6_CELTMode *mode = st->mode;
         int C = mode->nbChannels;

         if (st->pitch_available > 0) st->pitch_available = 1;

         cc6_CELT_MEMSET(st->in_mem, 0, st->overlap*C);
         cc6_CELT_MEMSET(st->out_mem, 0, (cc6_MAX_PERIOD+st->overlap)*C);

         cc6_CELT_MEMSET(st->oldBandE, 0, C*mode->nbEBands);

         cc6_CELT_MEMSET(st->preemph_memE, 0, C);
         cc6_CELT_MEMSET(st->preemph_memD, 0, C);
         st->delayedIntra = 1;
      }
      break;
      default:
         goto bad_request;
   }
   va_end(ap);
   return cc6_CELT_OK;
bad_mode:
  va_end(ap);
  return cc6_CELT_INVALID_MODE;
bad_arg:
   va_end(ap);
   return cc6_CELT_BAD_ARG;
bad_request:
   va_end(ap);
   return cc6_CELT_UNIMPLEMENTED;
}

/**********************************************************************/
/*                                                                    */
/*                             DECODER                                */
/*                                                                    */
/**********************************************************************/
#ifdef NEW_PLC
#define cc6_DECODE_BUFFER_SIZE 2048
#else
#define cc6_DECODE_BUFFER_SIZE cc6_MAX_PERIOD
#endif

#define cc6_DECODERVALID   0x4c434454
#define cc6_DECODERPARTIAL 0x5444434c
#define cc6_DECODERFREED   0x4c004400

/** Decoder state 
 @brief Decoder state
 */
struct cc6_CELTDecoder {
   cc6_celt_uint32_t marker;
   const cc6_CELTMode *mode;
   int frame_size;
   int block_size;
   int overlap;

   cc6_ec_byte_buffer buf;
   cc6_ec_enc         enc;

   cc6_celt_sig_t * __restrict preemph_memD;

   cc6_celt_sig_t *out_mem;
   cc6_celt_sig_t *decode_mem;

   cc6_celt_word16_t *oldBandE;
   
   int last_pitch_index;
   int loss_count;
};

int cc6_check_decoder(const cc6_CELTDecoder *st)
{
   if (st==NULL)
   {
      cc6_celt_warning("NULL passed a decoder structure");
      return cc6_CELT_INVALID_STATE;
   }
   if (st->marker == cc6_DECODERVALID)
      return cc6_CELT_OK;
   if (st->marker == cc6_DECODERFREED)
      cc6_celt_warning("Referencing a decoder that has already been freed");
   else
      cc6_celt_warning("This is not a valid CELT decoder structure");
   return cc6_CELT_INVALID_STATE;
}

cc6_CELTDecoder *cc6_celt_decoder_create(const cc6_CELTMode *mode)
{
   int N, C;
   cc6_CELTDecoder *st;

   if (cc6_check_mode(mode) != cc6_CELT_OK)
      return NULL;

   N = mode->mdctSize;
   C = cc6_CHANNELS(mode);
   st = cc6_celt_alloc(sizeof(cc6_CELTDecoder));

   if (st==NULL)
      return NULL;
   
   st->marker = cc6_DECODERPARTIAL;
   st->mode = mode;
   st->frame_size = N;
   st->block_size = N;
   st->overlap = mode->overlap;

   st->decode_mem = cc6_celt_alloc((cc6_DECODE_BUFFER_SIZE+st->overlap)*C*sizeof(cc6_celt_sig_t));
   st->out_mem = st->decode_mem+cc6_DECODE_BUFFER_SIZE-cc6_MAX_PERIOD;
   
   st->oldBandE = (cc6_celt_word16_t*)cc6_celt_alloc(C*mode->nbEBands*sizeof(cc6_celt_word16_t));
   
   st->preemph_memD = (cc6_celt_sig_t*)cc6_celt_alloc(C*sizeof(cc6_celt_sig_t));

   st->loss_count = 0;

   if ((st->decode_mem!=NULL) && (st->out_mem!=NULL) && (st->oldBandE!=NULL) &&
       (st->preemph_memD!=NULL))
   {
      st->marker = cc6_DECODERVALID;
      return st;
   }
   /* If the setup fails for some reason deallocate it. */
   cc6_celt_decoder_destroy(st);
   return NULL;
}

void cc6_celt_decoder_destroy(cc6_CELTDecoder *st)
{
   if (st == NULL)
   {
      cc6_celt_warning("NULL passed to cc6_celt_decoder_destroy");
      return;
   }

   if (st->marker == cc6_DECODERFREED)
   {
      cc6_celt_warning("Freeing a decoder which has already been freed");
      return;
   }
   
   if (st->marker != cc6_DECODERVALID && st->marker != cc6_DECODERPARTIAL)
   {
      cc6_celt_warning("This is not a valid CELT decoder structure");
      return;
   }
   
   /*Check_mode is non-fatal here because we can still free
     the encoder memory even if the mode is bad, although calling
     the free functions in this order is a violation of the API.*/
   cc6_check_mode(st->mode);
   
   cc6_celt_free(st->decode_mem);
   cc6_celt_free(st->oldBandE);
   cc6_celt_free(st->preemph_memD);
   
   st->marker = cc6_DECODERFREED;
   
   cc6_celt_free(st);
}

/** Handles lost packets by just copying past data with the same
    offset as the last
    pitch period */
#ifdef NEW_PLC
#include "plc.c"
#else
static void cc6_celt_decode_lost(cc6_CELTDecoder * __restrict st, cc6_celt_word16_t * __restrict pcm)
{
   int c, N;
   int pitch_index;
   cc6_celt_word16_t fade = cc6_Q15ONE;
   int i, len;
   cc6_VARDECL(cc6_celt_sig_t, freq);
   const int C = cc6_CHANNELS(st->mode);
   int offset;
   cc6_SAVE_STACK;
   N = st->block_size;
   cc6_ALLOC(freq,C*N, cc6_celt_sig_t); /**< Interleaved signal MDCTs */
   
   len = N+st->mode->overlap;
   
   if (st->loss_count == 0)
   {
      cc6_find_spectral_pitch(st->mode, st->mode->fft, &st->mode->psy, st->out_mem+cc6_MAX_PERIOD-len, st->out_mem, st->mode->window, NULL, len, cc6_MAX_PERIOD-len-100, &pitch_index);
      pitch_index = cc6_MAX_PERIOD-len-pitch_index;
      st->last_pitch_index = pitch_index;
   } else {
      pitch_index = st->last_pitch_index;
      if (st->loss_count < 10)
         fade = cc6_QCONST16(.85f,15);
      else
         fade = 0;
   }

   offset = cc6_MAX_PERIOD-pitch_index;
   while (offset+len >= cc6_MAX_PERIOD)
      offset -= pitch_index;
   cc6_compute_mdcts(st->mode, 0, st->out_mem+offset*C, freq);
   for (i=0;i<C*N;i++)
      freq[i] = cc6_ADD32(cc6_VERY_SMALL, cc6_MULT16_32_Q15(fade,freq[i]));

   cc6_CELT_MOVE(st->out_mem, st->out_mem+C*N, C*(cc6_MAX_PERIOD+st->mode->overlap-N));
   /* Compute inverse MDCTs */
   cc6_compute_inv_mdcts(st->mode, 0, freq, -1, 0, st->out_mem);

   for (c=0;c<C;c++)
   {
      int j;
      for (j=0;j<N;j++)
      {
         cc6_celt_sig_t tmp = cc6_MAC16_32_Q15(st->out_mem[C*(cc6_MAX_PERIOD-N)+C*j+c],
                                cc6_preemph,st->preemph_memD[c]);
         st->preemph_memD[c] = tmp;
         pcm[C*j+c] = cc6_SCALEOUT(cc6_SIG2WORD16(tmp));
      }
   }
   
   st->loss_count++;

   cc6_RESTORE_STACK;
}
#endif

#ifdef FIXED_POINT
int cc6_celt_decode(cc6_CELTDecoder * __restrict st, const unsigned char *data, int len, cc6_celt_int16_t * __restrict pcm)
{
#else
int cc6_celt_decode_float(cc6_CELTDecoder * __restrict st, const unsigned char *data, int len, cc6_celt_sig_t * __restrict pcm)
{
#endif
   int i, c, N, N4;
   int has_pitch, has_fold;
   int pitch_index;
   int bits;
   cc6_ec_dec dec;
   cc6_ec_byte_buffer buf;
   cc6_VARDECL(cc6_celt_sig_t, freq);
   cc6_VARDECL(cc6_celt_norm_t, X);
   cc6_VARDECL(cc6_celt_norm_t, P);
   cc6_VARDECL(cc6_celt_ener_t, bandE);
   cc6_VARDECL(cc6_celt_pgain_t, gains);
   cc6_VARDECL(int, fine_quant);
   cc6_VARDECL(int, pulses);
   cc6_VARDECL(int, offsets);
   cc6_VARDECL(int, fine_priority);

   int shortBlocks;
   int intra_ener;
   int transient_time;
   int transient_shift;
   int mdct_weight_shift=0;
   const int C = cc6_CHANNELS(st->mode);
   int mdct_weight_pos=0;
   cc6_SAVE_STACK;

   if (cc6_check_decoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (pcm==NULL)
      return cc6_CELT_BAD_ARG;

   N = st->block_size;
   N4 = (N-st->overlap)>>1;

   cc6_ALLOC(freq, C*N, cc6_celt_sig_t); /**< Interleaved signal MDCTs */
   cc6_ALLOC(X, C*N, cc6_celt_norm_t);   /**< Interleaved normalised MDCTs */
   cc6_ALLOC(P, C*N, cc6_celt_norm_t);   /**< Interleaved normalised pitch MDCTs*/
   cc6_ALLOC(bandE, st->mode->nbEBands*C, cc6_celt_ener_t);
   cc6_ALLOC(gains, st->mode->nbPBands, cc6_celt_pgain_t);
   
   if (data == NULL)
   {
      cc6_celt_decode_lost(st, pcm);
      cc6_RESTORE_STACK;
      return 0;
   } else {
      st->loss_count = 0;
   }
   if (len<0) {
     cc6_RESTORE_STACK;
     return cc6_CELT_BAD_ARG;
   }
   
   cc6_ec_byte_readinit(&buf,(unsigned char*)data,len);
   cc6_ec_dec_init(&dec,&buf);
   
   cc6_decode_flags(&dec, &intra_ener, &has_pitch, &shortBlocks, &has_fold);
   if (shortBlocks)
   {
      transient_shift = cc6_ec_dec_bits(&dec, 2);
      if (transient_shift == 3)
      {
         transient_time = cc6_ec_dec_uint(&dec, N+st->mode->overlap);
      } else {
         mdct_weight_shift = transient_shift;
         if (mdct_weight_shift && st->mode->nbShortMdcts>2)
            mdct_weight_pos = cc6_ec_dec_uint(&dec, st->mode->nbShortMdcts-1);
         transient_shift = 0;
         transient_time = 0;
      }
   } else {
      transient_time = -1;
      transient_shift = 0;
   }
   
   if (has_pitch)
   {
      pitch_index = cc6_ec_dec_uint(&dec, cc6_MAX_PERIOD-(2*N-2*N4));
   } else {
      pitch_index = 0;
      for (i=0;i<st->mode->nbPBands;i++)
         gains[i] = 0;
   }

   cc6_ALLOC(fine_quant, st->mode->nbEBands, int);
   /* Get band energies */
   cc6_unquant_coarse_energy(st->mode, bandE, st->oldBandE, len*8/3, intra_ener, st->mode->prob, &dec);
   
   cc6_ALLOC(pulses, st->mode->nbEBands, int);
   cc6_ALLOC(offsets, st->mode->nbEBands, int);
   cc6_ALLOC(fine_priority, st->mode->nbEBands, int);

   for (i=0;i<st->mode->nbEBands;i++)
      offsets[i] = 0;

   bits = len*8 - cc6_ec_dec_tell(&dec, 0) - 1;
   if (has_pitch)
      bits -= st->mode->nbPBands;
   cc6_compute_allocation(st->mode, offsets, bits, pulses, fine_quant, fine_priority);
   /*bits = cc6_ec_dec_tell(&dec, 0);
   cc6_compute_fine_allocation(st->mode, fine_quant, (20*C+len*8/5-(cc6_ec_dec_tell(&dec, 0)-bits))/C);*/
   
   cc6_unquant_fine_energy(st->mode, bandE, st->oldBandE, fine_quant, &dec);


   if (has_pitch) 
   {
      cc6_VARDECL(cc6_celt_ener_t, bandEp);
      
      /* Pitch MDCT */
      cc6_compute_mdcts(st->mode, 0, st->out_mem+pitch_index*C, freq);
      cc6_ALLOC(bandEp, st->mode->nbEBands*C, cc6_celt_ener_t);
      cc6_compute_band_energies(st->mode, freq, bandEp);
      cc6_normalise_bands(st->mode, freq, P, bandEp);
      /* Apply pitch gains */
   } else {
      for (i=0;i<C*N;i++)
         P[i] = 0;
   }

   /* Decode fixed codebook and merge with pitch */
   if (C==1)
      cc6_unquant_bands(st->mode, X, P, has_pitch, gains, bandE, pulses, shortBlocks, has_fold, len*8, &dec);
#ifndef DISABLE_STEREO
   else
      cc6_unquant_bands_stereo(st->mode, X, P, has_pitch, gains, bandE, pulses, shortBlocks, has_fold, len*8, &dec);
#endif
   cc6_unquant_energy_finalise(st->mode, bandE, st->oldBandE, fine_quant, fine_priority, len*8-cc6_ec_dec_tell(&dec, 0), &dec);
   
   /* Synthesis */
   cc6_denormalise_bands(st->mode, X, freq, bandE);


   cc6_CELT_MOVE(st->decode_mem, st->decode_mem+C*N, C*(cc6_DECODE_BUFFER_SIZE+st->overlap-N));
   if (mdct_weight_shift)
   {
      int m;
      for (c=0;c<C;c++)
         for (m=mdct_weight_pos+1;m<st->mode->nbShortMdcts;m++)
            for (i=m+c*N;i<(c+1)*N;i+=st->mode->nbShortMdcts)
#ifdef FIXED_POINT
               freq[i] = cc6_SHL32(freq[i], mdct_weight_shift);
#else
               freq[i] = (1<<mdct_weight_shift)*freq[i];
#endif
   }
   /* Compute inverse MDCTs */
   cc6_compute_inv_mdcts(st->mode, shortBlocks, freq, transient_time, transient_shift, st->out_mem);

   for (c=0;c<C;c++)
   {
      int j;
      for (j=0;j<N;j++)
      {
         cc6_celt_sig_t tmp = cc6_MAC16_32_Q15(st->out_mem[C*(cc6_MAX_PERIOD-N)+C*j+c],
                                cc6_preemph,st->preemph_memD[c]);
         st->preemph_memD[c] = tmp;
         pcm[C*j+c] = cc6_SCALEOUT(cc6_SIG2WORD16(tmp));
      }
   }

   cc6_RESTORE_STACK;
   return 0;
}

#ifdef FIXED_POINT
#ifndef DISABLE_FLOAT_API
int cc6_celt_decode_float(cc6_CELTDecoder * __restrict st, const unsigned char *data, int len, float * __restrict pcm)
{
   int j, ret, C, N;
   cc6_VARDECL(cc6_celt_int16_t, out);

   if (cc6_check_decoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (pcm==NULL)
      return cc6_CELT_BAD_ARG;

   cc6_SAVE_STACK;
   C = cc6_CHANNELS(st->mode);
   N = st->block_size;
   
   cc6_ALLOC(out, C*N, cc6_celt_int16_t);
   ret=cc6_celt_decode(st, data, len, out);
   for (j=0;j<C*N;j++)
      pcm[j]=out[j]*(1/32768.);
     
   cc6_RESTORE_STACK;
   return ret;
}
#endif /*DISABLE_FLOAT_API*/
#else
int cc6_celt_decode(cc6_CELTDecoder * __restrict st, const unsigned char *data, int len, cc6_celt_int16_t * __restrict pcm)
{
   int j, ret, C, N;
   cc6_VARDECL(cc6_celt_sig_t, out);

   if (cc6_check_decoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   if (cc6_check_mode(st->mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;

   if (pcm==NULL)
      return cc6_CELT_BAD_ARG;

   cc6_SAVE_STACK;
   C = cc6_CHANNELS(st->mode);
   N = st->block_size;
   cc6_ALLOC(out, C*N, cc6_celt_sig_t);

   ret=cc6_celt_decode_float(st, data, len, out);

   for (j=0;j<C*N;j++)
      pcm[j] = cc6_FLOAT2INT16 (out[j]);
   
   cc6_RESTORE_STACK;
   return ret;
}
#endif

int cc6_celt_decoder_ctl(cc6_CELTDecoder * __restrict st, int request, ...)
{
   va_list ap;

   if (cc6_check_decoder(st) != cc6_CELT_OK)
      return cc6_CELT_INVALID_STATE;

   va_start(ap, request);
   if ((request!=cc6_CELT_GET_MODE_REQUEST) && (cc6_check_mode(st->mode) != cc6_CELT_OK))
     goto bad_mode;
   switch (request)
   {
      case cc6_CELT_GET_MODE_REQUEST:
      {
         const cc6_CELTMode ** value = va_arg(ap, const cc6_CELTMode**);
         if (value==0)
            goto bad_arg;
         *value=st->mode;
      }
      break;
      case cc6_CELT_RESET_STATE:
      {
         const cc6_CELTMode *mode = st->mode;
         int C = mode->nbChannels;

         cc6_CELT_MEMSET(st->decode_mem, 0, (cc6_DECODE_BUFFER_SIZE+st->overlap)*C);
         cc6_CELT_MEMSET(st->oldBandE, 0, C*mode->nbEBands);

         cc6_CELT_MEMSET(st->preemph_memD, 0, C);

         st->loss_count = 0;
      }
      break;
      default:
         goto bad_request;
   }
   va_end(ap);
   return cc6_CELT_OK;
bad_mode:
  va_end(ap);
  return cc6_CELT_INVALID_MODE;
bad_arg:
   va_end(ap);
   return cc6_CELT_BAD_ARG;
bad_request:
      va_end(ap);
  return cc6_CELT_UNIMPLEMENTED;
}
