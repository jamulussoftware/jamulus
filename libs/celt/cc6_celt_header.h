/* (C) 2008 Jean-Marc Valin, CSIRO
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

#ifndef cc6_CELT_HEADER_H
#define cc6_CELT_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cc6_celt.h"
#include "cc6_celt_types.h"

/** Header data to be used for Ogg files (or possibly other encapsulation) 
    @brief Header data 
 */
typedef struct {
   char             codec_id[8];       /**< MUST be "CELT    " (four spaces) */
   char             codec_version[20]; /**< Version used (as string) */
   cc6_celt_int32_t version_id;        /**< Version id (negative for until stream is frozen) */
   cc6_celt_int32_t header_size;       /**< Size of this header */
   cc6_celt_int32_t sample_rate;       /**< Sampling rate of the original audio */
   cc6_celt_int32_t nb_channels;       /**< Number of channels */
   cc6_celt_int32_t frame_size;        /**< Samples per frame (per channel) */
   cc6_celt_int32_t overlap;           /**< Overlapping samples (per channel) */
   cc6_celt_int32_t bytes_per_packet;  /**< Number of bytes per compressed packet (0 if unknown) */
   cc6_celt_int32_t extra_headers;     /**< Number of additional headers that follow this header */
} cc6_CELTHeader;

/** Creates a basic header struct */
cc6_EXPORT int cc6_celt_header_init(cc6_CELTHeader *header, const cc6_CELTMode *m);

cc6_EXPORT int cc6_celt_header_to_packet(const cc6_CELTHeader *header, unsigned char *packet, cc6_celt_uint32_t size);

cc6_EXPORT int cc6_celt_header_from_packet(const unsigned char *packet, cc6_celt_uint32_t size, cc6_CELTHeader *header);

#ifdef __cplusplus
}
#endif

#endif /* cc6_CELT_HEADER_H */
