/* (C) 2007-2009 Jean-Marc Valin, CSIRO
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

#include "cc6_celt.h"
#include "cc6_modes.h"
#include "cc6_rate.h"
#include "cc6_os_support.h"
#include "cc6_stack_alloc.h"
#include "cc6_quant_bands.h"

#ifdef STATIC_MODES
#include "static_modes.c"
#endif

#define cc6_MODEVALID   0xa110ca7e
#define cc6_MODEPARTIAL 0x7eca10a1
#define cc6_MODEFREED   0xb10cf8ee

#ifndef cc6_M_PI
#define cc6_M_PI 3.141592653
#endif


int cc6_celt_mode_info(const cc6_CELTMode *mode, int request, cc6_celt_int32_t *value)
{
   if (cc6_check_mode(mode) != cc6_CELT_OK)
      return cc6_CELT_INVALID_MODE;
   switch (request)
   {
      case cc6_CELT_GET_FRAME_SIZE:
         *value = mode->mdctSize;
         break;
      case cc6_CELT_GET_LOOKAHEAD:
         *value = mode->overlap;
         break;
      case cc6_CELT_GET_NB_CHANNELS:
         *value = mode->nbChannels;
         break;
      case cc6_CELT_GET_BITSTREAM_VERSION:
         *value = cc6_CELT_BITSTREAM_VERSION;
         break;
      case cc6_CELT_GET_SAMPLE_RATE:
         *value = mode->Fs;
         break;
      default:
         return cc6_CELT_UNIMPLEMENTED;
   }
   return cc6_CELT_OK;
}

#ifndef STATIC_MODES

#define cc6_PBANDS 8

#ifdef STDIN_TUNING
int cc6_MIN_BINS;
#else
#define cc6_MIN_BINS 3
#endif

/* Defining 25 critical bands for the full 0-20 kHz audio bandwidth
   Taken from http://ccrma.stanford.edu/~jos/bbt/Bark_Frequency_Scale.html */
#define cc6_BARK_BANDS 25
static const cc6_celt_int16_t bark_freq[cc6_BARK_BANDS+1] = {
      0,   100,   200,   300,   400,
    510,   630,   770,   920,  1080,
   1270,  1480,  1720,  2000,  2320,
   2700,  3150,  3700,  4400,  5300,
   6400,  7700,  9500, 12000, 15500,
  20000};

static const cc6_celt_int16_t pitch_freq[cc6_PBANDS+1] ={0, 345, 689, 1034, 1378, 2067, 3273, 5340, 6374};

/* This allocation table is per critical band. When creating a mode, the bits get added together 
   into the codec bands, which are sometimes larger than one critical band at low frequency */

#ifdef STDIN_TUNING
int cc6_BITALLOC_SIZE;
int *cc6_band_allocation;
#else
#define cc6_BITALLOC_SIZE 12
static const int cc6_band_allocation[cc6_BARK_BANDS*cc6_BITALLOC_SIZE] =
   /* 0 100 200 300 400 510 630 770 920 1k  1.2 1.5 1.7 2k  2.3 2.7 3.1 3.7 4.4 5.3 6.4 7.7 9.5 12k 15k  */
   {  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*0*/
      2,  2,  1,  1,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*1*/
      2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  2,  2,  4,  5,  7,  7,  7,  5,  4,  0,  0,  0,  0,  0,  0, /*2*/
      2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  5,  6,  8,  8,  8,  6,  5,  4,  0,  0,  0,  0,  0, /*3*/
      3,  2,  2,  2,  3,  4,  4,  4,  4,  4,  4,  4,  6,  7,  9,  9,  9,  7,  6,  5,  5,  5,  0,  0,  0, /*4*/
      3,  3,  3,  4,  4,  5,  6,  6,  6,  6,  6,  7,  7,  9, 10, 10, 10,  9,  6,  5,  5,  5,  5,  1,  0, /*5*/
      4,  3,  3,  4,  6,  7,  7,  7,  7,  7,  8,  9,  9,  9, 11, 10, 10,  9,  9,  8, 11, 10, 10,  1,  0, /*6*/
      5,  5,  5,  6,  7,  7,  7,  7,  8,  8,  9, 10, 10, 12, 12, 11, 11, 17, 12, 15, 15, 20, 18, 10,  1, /*7*/
      6,  7,  7,  7,  8,  8,  8,  8,  9, 10, 11, 12, 14, 17, 18, 21, 22, 27, 29, 39, 37, 38, 40, 35,  1, /*8*/
      7,  7,  7,  8,  8,  8, 10, 10, 10, 13, 14, 18, 20, 24, 28, 32, 32, 35, 38, 38, 42, 50, 59, 54, 31, /*9*/
      8,  8,  8,  8,  8,  9, 10, 12, 14, 20, 22, 25, 28, 30, 35, 42, 46, 50, 55, 60, 62, 62, 72, 82, 62, /*10*/
      9,  9,  9, 10, 12, 13, 15, 18, 22, 30, 32, 35, 40, 45, 55, 62, 66, 70, 85, 90, 92, 92, 92,102, 92, /*11*/
   };
#endif

static cc6_celt_int16_t *cc6_compute_ebands(cc6_celt_int32_t Fs, int frame_size, int *nbEBands)
{
   cc6_celt_int16_t *eBands;
   int i, res, min_width, lin, low, high, nBark;
   res = (Fs+frame_size)/(2*frame_size);
   min_width = cc6_MIN_BINS*res;

   /* Find the number of critical bands supported by our sampling rate */
   for (nBark=1;nBark<cc6_BARK_BANDS;nBark++)
    if (bark_freq[nBark+1]*2 >= Fs)
       break;

   /* Find where the linear part ends (i.e. where the spacing is more than min_width */
   for (lin=0;lin<nBark;lin++)
      if (bark_freq[lin+1]-bark_freq[lin] >= min_width)
         break;
   
   low = ((bark_freq[lin]/res)+(cc6_MIN_BINS-1))/cc6_MIN_BINS;
   high = nBark-lin;
   *nbEBands = low+high;
   eBands = cc6_celt_alloc(sizeof(cc6_celt_int16_t)*(*nbEBands+2));
   
   if (eBands==NULL)
      return NULL;
   
   /* Linear spacing (min_width) */
   for (i=0;i<low;i++)
      eBands[i] = cc6_MIN_BINS*i;
   /* Spacing follows critical bands */
   for (i=0;i<high;i++)
      eBands[i+low] = (bark_freq[lin+i]+res/2)/res;
   /* Enforce the minimum spacing at the boundary */
   for (i=0;i<*nbEBands;i++)
      if (eBands[i] < cc6_MIN_BINS*i)
         eBands[i] = cc6_MIN_BINS*i;
   eBands[*nbEBands] = (bark_freq[nBark]+res/2)/res;
   eBands[*nbEBands+1] = frame_size;
   if (eBands[*nbEBands] > eBands[*nbEBands+1])
      eBands[*nbEBands] = eBands[*nbEBands+1];
   
   /* FIXME: Remove last band if too small */
   return eBands;
}

static void cc6_compute_pbands(cc6_CELTMode *mode, int res)
{
   int i;
   cc6_celt_int16_t *pBands;
   pBands=cc6_celt_alloc(sizeof(cc6_celt_int16_t)*(cc6_PBANDS+2));
   mode->pBands = pBands;
   if (pBands==NULL)
     return;
   mode->nbPBands = cc6_PBANDS;
   for (i=0;i<cc6_PBANDS+1;i++)
   {
      pBands[i] = (pitch_freq[i]+res/2)/res;
      if (pBands[i] < mode->eBands[i])
         pBands[i] = mode->eBands[i];
   }
   pBands[cc6_PBANDS+1] = mode->eBands[mode->nbEBands+1];
   for (i=1;i<mode->nbPBands+1;i++)
   {
      int j;
      for (j=0;j<mode->nbEBands;j++)
         if (mode->eBands[j] <= pBands[i] && mode->eBands[j+1] > pBands[i])
            break;
      if (mode->eBands[j] != pBands[i])
      {
         if (pBands[i]-mode->eBands[j] < mode->eBands[j+1]-pBands[i] && 
             mode->eBands[j] != pBands[i-1])
            pBands[i] = mode->eBands[j];
         else
            pBands[i] = mode->eBands[j+1];
      }
   }
   mode->pitchEnd = pBands[cc6_PBANDS];
}

static void cc6_compute_allocation_table(cc6_CELTMode *mode, int res)
{
   int i, j, nBark;
   cc6_celt_int16_t *allocVectors;
   const int C = cc6_CHANNELS(mode);

   /* Find the number of critical bands supported by our sampling rate */
   for (nBark=1;nBark<cc6_BARK_BANDS;nBark++)
    if (bark_freq[nBark+1]*2 >= mode->Fs)
       break;

   mode->nbAllocVectors = cc6_BITALLOC_SIZE;
   allocVectors = cc6_celt_alloc(sizeof(cc6_celt_int16_t)*(cc6_BITALLOC_SIZE*mode->nbEBands));
   if (allocVectors==NULL)
      return;
   /* Compute per-codec-band allocation from per-critical-band matrix */
   for (i=0;i<cc6_BITALLOC_SIZE;i++)
   {
      cc6_celt_int32_t current = 0;
      int eband = 0;
      for (j=0;j<nBark;j++)
      {
         int edge, low;
         cc6_celt_int32_t alloc;
         edge = mode->eBands[eband+1]*res;
         alloc = cc6_band_allocation[i*cc6_BARK_BANDS+j];
         alloc = alloc*C*mode->mdctSize;
         if (edge < bark_freq[j+1])
         {
            int num, den;
            num = alloc * (edge-bark_freq[j]);
            den = bark_freq[j+1]-bark_freq[j];
            low = (num+den/2)/den;
            allocVectors[i*mode->nbEBands+eband] = (current+low+128)/256;
            current=0;
            eband++;
            current += alloc-low;
         } else {
            current += alloc;
         }   
      }
      allocVectors[i*mode->nbEBands+eband] = (current+128)/256;
   }
   mode->allocVectors = allocVectors;
}

#endif /* STATIC_MODES */

cc6_CELTMode *cc6_celt_mode_create(cc6_celt_int32_t Fs, int channels, int frame_size, int *error)
{
   int i;
#ifdef STDIN_TUNING
   scanf("%d ", &cc6_MIN_BINS);
   scanf("%d ", &cc6_BITALLOC_SIZE);
   cc6_band_allocation = cc6_celt_alloc(sizeof(int)*cc6_BARK_BANDS*cc6_BITALLOC_SIZE);
   for (i=0;i<cc6_BARK_BANDS*cc6_BITALLOC_SIZE;i++)
   {
      scanf("%d ", cc6_band_allocation+i);
   }
#endif
#ifdef STATIC_MODES
   const cc6_CELTMode *m = NULL;
   cc6_CELTMode *mode=NULL;
   cc6_ALLOC_STACK;
#if !defined(VAR_ARRAYS) && !defined(USE_ALLOCA)
   if (cc6_global_stack==NULL)
   {
      cc6_celt_free(cc6_global_stack);
      goto failure;
   }
#endif 
   for (i=0;i<TOTAL_MODES;i++)
   {
      if (Fs == static_mode_list[i]->Fs &&
          channels == static_mode_list[i]->nbChannels &&
          frame_size == static_mode_list[i]->mdctSize)
      {
         m = static_mode_list[i];
         break;
      }
   }
   if (m == NULL)
   {
      cc6_celt_warning("Mode not included as part of the static modes");
      if (error)
         *error = cc6_CELT_BAD_ARG;
      return NULL;
   }
   mode = (cc6_CELTMode*)cc6_celt_alloc(sizeof(cc6_CELTMode));
   if (mode==NULL)
      goto failure;
   cc6_CELT_COPY(mode, m, 1);
   mode->marker_start = cc6_MODEPARTIAL;
#else
   int res;
   cc6_CELTMode *mode=NULL;
   cc6_celt_word16_t *window;
   cc6_ALLOC_STACK;
#if !defined(VAR_ARRAYS) && !defined(USE_ALLOCA)
   if (cc6_global_stack==NULL)
   {
      cc6_celt_free(cc6_global_stack);
      goto failure;
   }
#endif 

   /* The good thing here is that permutation of the arguments will automatically be invalid */
   
   if (Fs < 32000 || Fs > 96000)
   {
      cc6_celt_warning("Sampling rate must be between 32 kHz and 96 kHz");
      if (error)
         *error = cc6_CELT_BAD_ARG;
      return NULL;
   }
   if (channels < 0 || channels > 2)
   {
      cc6_celt_warning("Only mono and stereo supported");
      if (error)
         *error = cc6_CELT_BAD_ARG;
      return NULL;
   }
   if (frame_size < 64 || frame_size > 1024 || frame_size%2!=0)
   {
      cc6_celt_warning("Only even frame sizes from 64 to 1024 are supported");
      if (error)
         *error = cc6_CELT_BAD_ARG;
      return NULL;
   }
   res = (Fs+frame_size)/(2*frame_size);
   
   mode = cc6_celt_alloc(sizeof(cc6_CELTMode));
   if (mode==NULL)
      goto failure;
   mode->marker_start = cc6_MODEPARTIAL;
   mode->Fs = Fs;
   mode->mdctSize = frame_size;
   mode->nbChannels = channels;
   mode->eBands = cc6_compute_ebands(Fs, frame_size, &mode->nbEBands);
   if (mode->eBands==NULL)
      goto failure;
   cc6_compute_pbands(mode, res);
   if (mode->pBands==NULL)
      goto failure;
   mode->ePredCoef = cc6_QCONST16(.8f,15);

   if (frame_size > 640 && (frame_size%16)==0)
   {
     mode->nbShortMdcts = 8;
   } else if (frame_size > 384 && (frame_size%8)==0)
   {
     mode->nbShortMdcts = 4;
   } else if (frame_size > 384 && (frame_size%10)==0)
   {
     mode->nbShortMdcts = 5;
   } else if (frame_size > 256 && (frame_size%6)==0)
   {
     mode->nbShortMdcts = 3;
   } else if (frame_size > 256 && (frame_size%8)==0)
   {
     mode->nbShortMdcts = 4;
   } else if (frame_size > 64 && (frame_size%4)==0)
   {
     mode->nbShortMdcts = 2;
   } else if (frame_size > 128 && (frame_size%6)==0)
   {
     mode->nbShortMdcts = 3;
   } else
   {
     mode->nbShortMdcts = 1;
   }

   /* Overlap must be divisible by 4 */
   if (mode->nbShortMdcts > 1)
      mode->overlap = ((frame_size/mode->nbShortMdcts)>>2)<<2; 
   else
      mode->overlap = (frame_size>>3)<<2;

   cc6_compute_allocation_table(mode, res);
   if (mode->allocVectors==NULL)
      goto failure;
   
   window = (cc6_celt_word16_t*)cc6_celt_alloc(mode->overlap*sizeof(cc6_celt_word16_t));
   if (window==NULL)
      goto failure;

#ifndef FIXED_POINT
   for (i=0;i<mode->overlap;i++)
      window[i] = cc6_Q15ONE*sin(.5*cc6_M_PI* sin(.5*cc6_M_PI*(i+.5)/mode->overlap) * sin(.5*cc6_M_PI*(i+.5)/mode->overlap));
#else
   for (i=0;i<mode->overlap;i++)
      window[i] = cc6_MIN32(32767,32768.*sin(.5*cc6_M_PI* sin(.5*cc6_M_PI*(i+.5)/mode->overlap) * sin(.5*cc6_M_PI*(i+.5)/mode->overlap)));
#endif
   mode->window = window;

   mode->bits = (const cc6_celt_int16_t **)cc6_compute_alloc_cache(mode, 1);
   if (mode->bits==NULL)
      goto failure;

#ifndef SHORTCUTS
   cc6_psydecay_init(&mode->psy, cc6_MAX_PERIOD/2, mode->Fs);
   if (mode->psy.decayR==NULL)
      goto failure;
#endif
   
#endif /* !STATIC_MODES */

#ifdef DISABLE_STEREO
   if (channels > 1)
   {
      cc6_celt_warning("Stereo support was disable from this build");
      if (error)
         *error = cc6_CELT_BAD_ARG;
      return NULL;
   }
#endif

   cc6_mdct_init(&mode->mdct, 2*mode->mdctSize);
   mode->fft = cc6_pitch_state_alloc(cc6_MAX_PERIOD);

   mode->shortMdctSize = mode->mdctSize/mode->nbShortMdcts;
   cc6_mdct_init(&mode->shortMdct, 2*mode->shortMdctSize);
   mode->shortWindow = mode->window;
   mode->prob = cc6_quant_prob_alloc(mode);
   if ((mode->mdct.trig==NULL) || (mode->mdct.kfft==NULL) || (mode->fft==NULL) ||
       (mode->shortMdct.trig==NULL) || (mode->shortMdct.kfft==NULL) || (mode->prob==NULL))
     goto failure;

   mode->marker_start = cc6_MODEVALID;
   mode->marker_end   = cc6_MODEVALID;
   if (error)
      *error = cc6_CELT_OK;
   return mode;
failure: 
   if (error)
      *error = cc6_CELT_INVALID_MODE;
   if (mode!=NULL)
      cc6_celt_mode_destroy(mode);
   return NULL;
}

void cc6_celt_mode_destroy(cc6_CELTMode *mode)
{
   int i;
   const cc6_celt_int16_t *prevPtr = NULL;
   if (mode == NULL)
   {
      cc6_celt_warning("NULL passed to cc6_celt_mode_destroy");
      return;
   }

   if (mode->marker_start == cc6_MODEFREED || mode->marker_end == cc6_MODEFREED)
   {
      cc6_celt_warning("Freeing a mode which has already been freed");
      return;
   }

   if (mode->marker_start != cc6_MODEVALID && mode->marker_start != cc6_MODEPARTIAL)
   {
      cc6_celt_warning("This is not a valid CELT mode structure");
      return;  
   }
   mode->marker_start = cc6_MODEFREED;
#ifndef STATIC_MODES
   if (mode->bits!=NULL)
   {
      for (i=0;i<mode->nbEBands;i++)
      {
         if (mode->bits[i] != prevPtr)
         {
            prevPtr = mode->bits[i];
            cc6_celt_free((int*)mode->bits[i]);
          }
      }
   }   
   cc6_celt_free((int**)mode->bits);
   cc6_celt_free((int*)mode->eBands);
   cc6_celt_free((int*)mode->pBands);
   cc6_celt_free((int*)mode->allocVectors);
   
   cc6_celt_free((cc6_celt_word16_t*)mode->window);

#ifndef SHORTCUTS
   cc6_psydecay_clear(&mode->psy);
#endif
#endif
   cc6_mdct_clear(&mode->mdct);
   cc6_mdct_clear(&mode->shortMdct);
   cc6_pitch_state_free(mode->fft);
   cc6_quant_prob_free(mode->prob);
   mode->marker_end = cc6_MODEFREED;
   cc6_celt_free((cc6_CELTMode *)mode);
}

int cc6_check_mode(const cc6_CELTMode *mode)
{
   if (mode==NULL)
      return cc6_CELT_INVALID_MODE;
   if (mode->marker_start == cc6_MODEVALID && mode->marker_end == cc6_MODEVALID)
      return cc6_CELT_OK;
   if (mode->marker_start == cc6_MODEFREED || mode->marker_end == cc6_MODEFREED)
      cc6_celt_warning("Using a mode that has already been freed");
   else
      cc6_celt_warning("This is not a valid CELT mode");
   return cc6_CELT_INVALID_MODE;
}
