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

#ifndef cc6_BANDS_H
#define cc6_BANDS_H

#include "cc6_arch.h"
#include "cc6_modes.h"
#include "cc6_entenc.h"
#include "cc6_entdec.h"
#include "cc6_rate.h"

/** Compute the amplitude (sqrt energy) in each of the bands 
 * @param m Mode data 
 * @param X Spectrum
 * @param bands Square root of the energy for each band (returned)
 */
void cc6_compute_band_energies(const cc6_CELTMode *m, const cc6_celt_sig_t *X, cc6_celt_ener_t *bands);

void cc6_compute_noise_energies(const cc6_CELTMode *m, const cc6_celt_sig_t *X, const cc6_celt_word16_t *tonality, cc6_celt_ener_t *bank);

/** Normalise each band of X such that the energy in each band is 
    equal to 1
 * @param m Mode data 
 * @param X Spectrum (returned normalised)
 * @param bands Square root of the energy for each band
 */
void cc6_normalise_bands(const cc6_CELTMode *m, const cc6_celt_sig_t * __restrict freq, cc6_celt_norm_t * __restrict X, const cc6_celt_ener_t *bands);

void cc6_renormalise_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X);

/** Denormalise each band of X to restore full amplitude
 * @param m Mode data 
 * @param X Spectrum (returned de-normalised)
 * @param bands Square root of the energy for each band
 */
void cc6_denormalise_bands(const cc6_CELTMode *m, const cc6_celt_norm_t * __restrict X, cc6_celt_sig_t * __restrict freq, const cc6_celt_ener_t *bands);

/** Compute the pitch predictor gain for each pitch band
 * @param m Mode data 
 * @param X Spectrum to predict
 * @param P Pitch vector (normalised)
 * @param gains Gain computed for each pitch band (returned)
 * @param bank Square root of the energy for each band
 */
int cc6_compute_pitch_gain(const cc6_CELTMode *m, const cc6_celt_norm_t *X, const cc6_celt_norm_t *P, cc6_celt_pgain_t *gains);

int cc6_folding_decision(const cc6_CELTMode *m, cc6_celt_norm_t *X, cc6_celt_word16_t *average, int *last_decision);

/** Quantisation/encoding of the residual spectrum
 * @param m Mode data 
 * @param X Residual (normalised)
 * @param P Pitch vector (normalised)
 * @param W Perceptual weighting
 * @param total_bits Total number of bits that can be used for the frame (including the ones already spent)
 * @param enc Entropy encoder
 */
void cc6_quant_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, cc6_celt_mask_t *W, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int time_domain, int fold, int total_bits, cc6_ec_enc *enc);

void cc6_quant_bands_stereo(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, cc6_celt_mask_t *W, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int time_domain, int fold, int total_bits, cc6_ec_enc *enc);

/** Decoding of the residual spectrum
 * @param m Mode data 
 * @param X Residual (normalised)
 * @param P Pitch vector (normalised)
 * @param total_bits Total number of bits that can be used for the frame (including the ones already spent)
 * @param dec Entropy decoder
*/
void cc6_unquant_bands(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int time_domain, int fold, int total_bits, cc6_ec_dec *dec);

void cc6_unquant_bands_stereo(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, cc6_celt_norm_t *P, int pitch_used, cc6_celt_pgain_t *pgains, const cc6_celt_ener_t *bandE, int *pulses, int time_domain, int fold, int total_bits, cc6_ec_dec *dec);

void cc6_stereo_decision(const cc6_CELTMode *m, cc6_celt_norm_t * __restrict X, int *stereo_mode, int len);

#endif /* cc6_BANDS_H */
