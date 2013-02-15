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

#ifndef cc6_QUANT_BANDS
#define cc6_QUANT_BANDS

#include "cc6_arch.h"
#include "cc6_modes.h"
#include "cc6_entenc.h"
#include "cc6_entdec.h"
#include "cc6_mathops.h"

static __inline cc6_celt_word16_t cc6_amp2Log(cc6_celt_word32_t amp)
{
    return cc6_celt_log2(cc6_MAX32(cc6_QCONST32(.001f,14),cc6_SHL32(amp,2)));
}

static __inline cc6_celt_word32_t cc6_log2Amp(cc6_celt_word16_t lg)
{
    return cc6_PSHR32(cc6_celt_exp2(cc6_SHL16(lg,3)),4);
}

int *cc6_quant_prob_alloc(const cc6_CELTMode *m);
void cc6_quant_prob_free(int *freq);

void cc6_compute_fine_allocation(const cc6_CELTMode *m, int *bits, int budget);

int cc6_intra_decision(cc6_celt_word16_t *eBands, cc6_celt_word16_t *oldEBands, int len);

unsigned cc6_quant_coarse_energy(const cc6_CELTMode *m, cc6_celt_word16_t *eBands, cc6_celt_word16_t *oldEBands, int budget, int intra, int *prob, cc6_celt_word16_t *error, cc6_ec_enc *enc);

void cc6_quant_fine_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, cc6_celt_word16_t *error, int *fine_quant, cc6_ec_enc *enc);

void cc6_quant_energy_finalise(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, cc6_celt_word16_t *error, int *fine_quant, int *fine_priority, int bits_left, cc6_ec_enc *enc);

void cc6_unquant_coarse_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int budget, int intra, int *prob, cc6_ec_dec *dec);

void cc6_unquant_fine_energy(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int *fine_quant, cc6_ec_dec *dec);

void cc6_unquant_energy_finalise(const cc6_CELTMode *m, cc6_celt_ener_t *eBands, cc6_celt_word16_t *oldEBands, int *fine_quant, int *fine_priority, int bits_left, cc6_ec_dec *dec);

#endif /* cc6_QUANT_BANDS */
