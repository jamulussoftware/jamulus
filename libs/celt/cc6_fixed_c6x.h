/* Copyright (C) 2008 CSIRO */
/**
   @file fixed_c6x.h
   @brief Fixed-point operations for the TI C6x DSP family
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

#ifndef cc6_FIXED_C6X_H
#define cc6_FIXED_C6X_H

#undef cc6_MULT16_16SU
#define cc6_MULT16_16SU(a,b) _mpysu(a,b)

#undef cc6_MULT_16_16
#define cc6_MULT_16_16(a,b) _mpy(a,b)

#define cc6_celt_ilog2(x) (30 - _norm(x))
#define cc6_OVERRIDE_CELT_ILOG2

#undef cc6_MULT16_32_Q15
#define cc6_MULT16_32_Q15(a,b) cc6_ADD32(cc6_SHL(_mpylh(a,b),1), cc6_SHR(_mpsu(a,b),15)

#if 0
#include "dsplib.h"

#undef cc6_MAX16
#define cc6_MAX16(a,b) _max(a,b)

#undef cc6_MIN16
#define cc6_MIN16(a,b) _min(a,b)

#undef cc6_MAX32
#define cc6_MAX32(a,b) _lmax(a,b)

#undef cc6_MIN32
#define cc6_MIN32(a,b) _lmin(a,b)

#undef cc6_VSHR32
#define cc6_VSHR32(a, shift) _lshl(a,-(shift))

#undef cc6_MULT16_16_Q15
#define cc6_MULT16_16_Q15(a,b) (_smpy(a,b))

#define cc6_celt_maxabs16(x, len) cc6_MAX16(maxval((DATA *)x, len),-minval((DATA *)x, len))
#define cc6_OVERRIDE_CELT_MAXABS16

#define cc6_OVERRIDE_FIND_MAX16
static inline int cc6_find_max16(cc6_celt_word16_t *x, int len)
{
   DATA max_corr16 = -cc6_VERY_LARGE16;
   DATA pitch16 = 0;
   maxvec((DATA *)x, len, &max_corr16, &pitch16);
   return pitch16;
}
#endif

#endif /* cc6_FIXED_C6X_H */
