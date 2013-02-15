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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cc6_celt_header.h"
#include "cc6_os_support.h"
#include "cc6_modes.h"

/*typedef struct {
   char         codec_id[8];
   char         codec_version[20];
   cc6_celt_int32_t version_id;
   cc6_celt_int32_t header_size;
   cc6_celt_int32_t mode;
   cc6_celt_int32_t sample_rate;
   cc6_celt_int32_t nb_channels;
   cc6_celt_int32_t bytes_per_packet;
   cc6_celt_int32_t extra_headers;
} cc6_CELTHeader;*/

static  cc6_celt_uint32_t
cc6__le_32 (cc6_celt_uint32_t i)
{
   cc6_celt_uint32_t ret=i;
#if !defined(__LITTLE_ENDIAN__) && ( defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__) )
   ret =  (i>>24);
   ret += (i>>8) & 0x0000ff00;
   ret += (i<<8) & 0x00ff0000;
   ret += (i<<24);
#endif
   return ret;
}

int cc6_celt_header_init(cc6_CELTHeader *header, const cc6_CELTMode *m)
{

   if (cc6_check_mode(m) != cc6_CELT_OK)
     return cc6_CELT_INVALID_MODE;
   if (header==NULL)
     return cc6_CELT_BAD_ARG;

   cc6_CELT_COPY(header->codec_id, "CELT    ", 8);
   cc6_CELT_COPY(header->codec_version, "experimental        ", 20);

   cc6_celt_mode_info(m, cc6_CELT_GET_BITSTREAM_VERSION, &header->version_id);
   header->header_size = 56;
   header->sample_rate = m->Fs;
   header->nb_channels = m->nbChannels;
   header->frame_size = m->mdctSize;
   header->overlap = m->overlap;
   header->bytes_per_packet = -1;
   header->extra_headers = 0;
   return cc6_CELT_OK;
}

int cc6_celt_header_to_packet(const cc6_CELTHeader *header, unsigned char *packet, cc6_celt_uint32_t size)
{
   cc6_celt_int32_t * h;

   if ((size < 56) || (header==NULL) || (packet==NULL))
     return cc6_CELT_BAD_ARG; /* FAIL */

   cc6_CELT_MEMSET(packet, 0, sizeof(*header));
   /* FIXME: Do it in an alignment-safe manner */

   /* Copy ident and version */
   cc6_CELT_COPY(packet, (unsigned char*)header, 28);

   /* Copy the int32 fields */
   h = (cc6_celt_int32_t*)(packet+28);
   *h++ = cc6__le_32 (header->version_id);
   *h++ = cc6__le_32 (header->header_size);
   *h++ = cc6__le_32 (header->sample_rate);
   *h++ = cc6__le_32 (header->nb_channels);
   *h++ = cc6__le_32 (header->frame_size);
   *h++ = cc6__le_32 (header->overlap);
   *h++ = cc6__le_32 (header->bytes_per_packet);
   *h   = cc6__le_32 (header->extra_headers);

   return sizeof(*header);
}

int cc6_celt_header_from_packet(const unsigned char *packet, cc6_celt_uint32_t size, cc6_CELTHeader *header)
{
   cc6_celt_int32_t * h;

   if ((size < 56) || (header==NULL) || (packet==NULL))
     return cc6_CELT_BAD_ARG; /* FAIL */

   cc6_CELT_MEMSET((unsigned char*)header, 0, sizeof(*header));
   /* FIXME: Do it in an alignment-safe manner */

   /* Copy ident and version */
   cc6_CELT_COPY((unsigned char*)header, packet, 28);

   /* Copy the int32 fields */
   h = (cc6_celt_int32_t*)(packet+28);
   header->version_id       = cc6__le_32(*h++);
   header->header_size      = cc6__le_32(*h++);
   header->sample_rate      = cc6__le_32(*h++);
   header->nb_channels      = cc6__le_32(*h++);
   header->frame_size       = cc6__le_32(*h++);
   header->overlap          = cc6__le_32(*h++);
   header->bytes_per_packet = cc6__le_32(*h++);
   header->extra_headers    = cc6__le_32(*h);

   return sizeof(*header);
}

