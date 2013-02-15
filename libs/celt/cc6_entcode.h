/* (C) 2001-2008 Timothy B. Terriberry
   (C) 2008 Jean-Marc Valin */
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

#include "cc6_celt_types.h"

#if !defined(cc6__entcode_H)
# define cc6__entcode_H (1)
# include <limits.h>
# include "cc6_ecintrin.h"



typedef cc6_celt_int32_t cc6_ec_int32;
typedef cc6_celt_uint32_t cc6_ec_uint32;
typedef cc6_celt_uint64_t cc6_ec_uint64;
typedef struct cc6_ec_byte_buffer cc6_ec_byte_buffer;



/*The number of bits to code at a time when coding bits directly.*/
# define cc6_EC_UNIT_BITS  (8)
/*The mask for the given bits.*/
# define cc6_EC_UNIT_MASK  ((1U<<cc6_EC_UNIT_BITS)-1)



/*Simple libogg1-style buffer.*/
struct cc6_ec_byte_buffer{
  unsigned char *buf;
  unsigned char *ptr;
  long           storage;
  int            resizable;
};

/*Encoding functions.*/
void cc6_ec_byte_writeinit_buffer(cc6_ec_byte_buffer *_b, unsigned char *_buf, long _size);
void cc6_ec_byte_writeinit(cc6_ec_byte_buffer *_b);
void cc6_ec_byte_writetrunc(cc6_ec_byte_buffer *_b,long _bytes);
void cc6_ec_byte_write1(cc6_ec_byte_buffer *_b,unsigned _value);
void cc6_ec_byte_write4(cc6_ec_byte_buffer *_b,cc6_ec_uint32 _value);
void cc6_ec_byte_writecopy(cc6_ec_byte_buffer *_b,void *_source,long _bytes);
void cc6_ec_byte_writeclear(cc6_ec_byte_buffer *_b);
/*Decoding functions.*/
void cc6_ec_byte_readinit(cc6_ec_byte_buffer *_b,unsigned char *_buf,long _bytes);
int cc6_ec_byte_look1(cc6_ec_byte_buffer *_b);
int cc6_ec_byte_look4(cc6_ec_byte_buffer *_b,cc6_ec_uint32 *_val);
void cc6_ec_byte_adv1(cc6_ec_byte_buffer *_b);
void cc6_ec_byte_adv4(cc6_ec_byte_buffer *_b);
int cc6_ec_byte_read1(cc6_ec_byte_buffer *_b);
int cc6_ec_byte_read4(cc6_ec_byte_buffer *_b,cc6_ec_uint32 *_val);
/*Shared functions.*/
static __inline void cc6_ec_byte_reset(cc6_ec_byte_buffer *_b){
   _b->ptr=_b->buf;
}

static __inline long cc6_ec_byte_bytes(cc6_ec_byte_buffer *_b){
   return _b->ptr-_b->buf;
}

static __inline unsigned char *cc6_ec_byte_get_buffer(cc6_ec_byte_buffer *_b){
   return _b->buf;
}

int cc6_ec_ilog(cc6_ec_uint32 _v);
int cc6_ec_ilog64(cc6_ec_uint64 _v);

#endif
