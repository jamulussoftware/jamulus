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

#ifndef cc6_MODES_H
#define cc6_MODES_H

#include "cc6_celt_types.h"
#include "cc6_celt.h"
#include "cc6_arch.h"
#include "cc6_mdct.h"
#include "cc6_psy.h"
#include "cc6_pitch.h"

#define cc6_CELT_BITSTREAM_VERSION 0x80000009

#ifdef STATIC_MODES
#include "static_modes.h"
#endif

#define cc6_MAX_PERIOD 1024

#ifndef cc6_CHANNELS
# ifdef DISABLE_STEREO
#  define cc6_CHANNELS(mode) (1)
# else
#  define cc6_CHANNELS(mode) ((mode)->nbChannels)
# endif
#endif

#define cc6_MDCT(mode) (&(mode)->mdct)

#ifndef cc6_OVERLAP
#define cc6_OVERLAP(mode) ((mode)->overlap)
#endif

#ifndef cc6_FRAMESIZE
#define cc6_FRAMESIZE(mode) ((mode)->mdctSize)
#endif

/** Mode definition (opaque)
 @brief Mode definition 
 */
struct cc6_CELTMode {
   cc6_celt_uint32_t marker_start;
   cc6_celt_int32_t Fs;
   int          overlap;
   int          mdctSize;
   int          nbChannels;
   
   int          nbEBands;
   int          nbPBands;
   int          pitchEnd;
   
   const cc6_celt_int16_t   *eBands;   /**< Definition for each "pseudo-critical band" */
   const cc6_celt_int16_t   *pBands;   /**< Definition of the bands used for the pitch */
   
   cc6_celt_word16_t ePredCoef;/**< Prediction coefficient for the energy encoding */
   
   int          nbAllocVectors; /**< Number of lines in the matrix below */
   const cc6_celt_int16_t   *allocVectors;   /**< Number of bits in each band for several rates */
   
   const cc6_celt_int16_t * const *bits; /**< Cache for pulses->bits mapping in each band */

   /* Stuff that could go in the {en,de}coder, but we save space this way */
   cc6_mdct_lookup mdct;
   cc6_kiss_fftr_cfg fft;

   const cc6_celt_word16_t *window;

   int         nbShortMdcts;
   int         shortMdctSize;
   cc6_mdct_lookup shortMdct;
   const cc6_celt_word16_t *shortWindow;

   struct cc6_PsyDecay psy;

   int *prob;
   cc6_celt_uint32_t marker_end;
};

int cc6_check_mode(const cc6_CELTMode *mode);

#endif
