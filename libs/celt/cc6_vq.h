/* (C) 2007-2008 Jean-Marc Valin, CSIRO
*/
/**
   @file vq.h
   @brief Vector quantisation of the residual
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

#ifndef cc6_VQ_H
#define cc6_VQ_H

#include "cc6_entenc.h"
#include "cc6_entdec.h"
#include "cc6_modes.h"

/** Algebraic pulse-vector quantiser. The signal x is replaced by the sum of 
  * the pitch and a combination of pulses such that its norm is still equal 
  * to 1. This is the function that will typically require the most CPU. 
 * @param x Residual signal to quantise/encode (returns quantised version)
 * @param W Perceptual weight to use when optimising (currently unused)
 * @param N Number of samples to encode
 * @param K Number of pulses to use
 * @param p Pitch vector (it is assumed that p+x is a unit vector)
 * @param enc Entropy encoder state
*/
void cc6_alg_quant(cc6_celt_norm_t *X, cc6_celt_mask_t *W, int N, int K, cc6_celt_norm_t *P, cc6_ec_enc *enc);

/** Algebraic pulse decoder
 * @param x Decoded normalised spectrum (returned)
 * @param N Number of samples to decode
 * @param K Number of pulses to use
 * @param p Pitch vector (automatically added to x)
 * @param dec Entropy decoder state
 */
void cc6_alg_unquant(cc6_celt_norm_t *X, int N, int K, cc6_celt_norm_t *P, cc6_ec_dec *dec);

cc6_celt_word16_t cc6_renormalise_vector(cc6_celt_norm_t *X, cc6_celt_word16_t value, int N, int stride);

/** Intra-frame predictor that matches a section of the current frame (at lower
 * frequencies) to encode the current band.
 * @param x Residual signal to quantise/encode (returns quantised version)
 * @param W Perceptual weight
 * @param N Number of samples to encode
 * @param K Number of pulses to use
 * @param Y Lower frequency spectrum to use, normalised to the same standard deviation
 * @param P Pitch vector (it is assumed that p+x is a unit vector)
 * @param B Stride (number of channels multiplied by the number of MDCTs per frame)
 * @param N0 Number of valid offsets
 */
void cc6_intra_fold(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict x, int N, int *pulses, cc6_celt_norm_t *Y, cc6_celt_norm_t * __restrict P, int N0, int B);

#endif /* cc6_VQ_H */
