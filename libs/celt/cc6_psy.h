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
#ifndef cc6_PSY_H
#define cc6_PSY_H

#include "cc6_arch.h"
#include "cc6_celt.h"

struct cc6_PsyDecay {
   /*cc6_celt_word16_t *decayL;*/
   const cc6_celt_word16_t * __restrict decayR;
};

/** Pre-compute the decay of the psycho-acoustic spreading function */
void cc6_psydecay_init(struct cc6_PsyDecay *decay, int len, cc6_celt_int32_t Fs);

/** Free the memory allocated for the spreading function */
void cc6_psydecay_clear(struct cc6_PsyDecay *decay);

/** Compute the masking curve for an input (DFT) spectrum X */
void cc6_compute_masking(const struct cc6_PsyDecay *decay, cc6_celt_word16_t *X, cc6_celt_mask_t *mask, int len);

/** Compute the masking curve for an input (MDCT) spectrum X */
void cc6_compute_mdct_masking(const struct cc6_PsyDecay *decay, cc6_celt_word32_t *X, cc6_celt_word16_t *tonality, cc6_celt_word16_t *long_window, cc6_celt_mask_t *mask, int len);

void cc6_compute_tonality(const cc6_CELTMode *m, cc6_celt_word16_t * __restrict X, cc6_celt_word16_t * mem, int len, cc6_celt_word16_t *tonality, int mdct_size);

#endif /* cc6_PSY_H */
