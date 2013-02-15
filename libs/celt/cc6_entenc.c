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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cc6_os_support.h"
#include "cc6_entenc.h"
#include "cc6_arch.h"


#define cc6_EC_BUFFER_INCREMENT (256)

void cc6_ec_byte_writeinit_buffer(cc6_ec_byte_buffer *_b, unsigned char *_buf, long _size){
  _b->ptr=_b->buf=_buf;
  _b->storage=_size;
  _b->resizable = 0;
}

void cc6_ec_byte_writeinit(cc6_ec_byte_buffer *_b){
  _b->ptr=_b->buf=cc6_celt_alloc(cc6_EC_BUFFER_INCREMENT*sizeof(char));
  _b->storage=cc6_EC_BUFFER_INCREMENT;
  _b->resizable = 1;
}

void cc6_ec_byte_writetrunc(cc6_ec_byte_buffer *_b,long _bytes){
  _b->ptr=_b->buf+_bytes;
}

void cc6_ec_byte_write1(cc6_ec_byte_buffer *_b,unsigned _value){
  ptrdiff_t endbyte;
  endbyte=_b->ptr-_b->buf;
  if(endbyte>=_b->storage){
    if (_b->resizable){
      _b->buf=cc6_celt_realloc(_b->buf,(_b->storage+cc6_EC_BUFFER_INCREMENT)*sizeof(char));
      _b->storage+=cc6_EC_BUFFER_INCREMENT;
      _b->ptr=_b->buf+endbyte;
    } else {
      cc6_celt_fatal("range encoder overflow\n");
    }
  }
  *(_b->ptr++)=(unsigned char)_value;
}

void cc6_ec_byte_write4(cc6_ec_byte_buffer *_b,cc6_ec_uint32 _value){
  ptrdiff_t endbyte;
  endbyte=_b->ptr-_b->buf;
  if(endbyte+4>_b->storage){
    if (_b->resizable){
      _b->buf=cc6_celt_realloc(_b->buf,(_b->storage+cc6_EC_BUFFER_INCREMENT)*sizeof(char));
      _b->storage+=cc6_EC_BUFFER_INCREMENT;
      _b->ptr=_b->buf+endbyte;
    } else {
      cc6_celt_fatal("range encoder overflow\n");
    }
  }
  *(_b->ptr++)=(unsigned char)_value;
  _value>>=8;
  *(_b->ptr++)=(unsigned char)_value;
  _value>>=8;
  *(_b->ptr++)=(unsigned char)_value;
  _value>>=8;
  *(_b->ptr++)=(unsigned char)_value;
}

void cc6_ec_byte_writecopy(cc6_ec_byte_buffer *_b,void *_source,long _bytes){
  ptrdiff_t endbyte;
  endbyte=_b->ptr-_b->buf;
  if(endbyte+_bytes>_b->storage){
    if (_b->resizable){
      _b->storage=endbyte+_bytes+cc6_EC_BUFFER_INCREMENT;
      _b->buf=cc6_celt_realloc(_b->buf,_b->storage*sizeof(char));
      _b->ptr=_b->buf+endbyte;
    } else {
      cc6_celt_fatal("range encoder overflow\n");
    }
  }
  memmove(_b->ptr,_source,_bytes);
  _b->ptr+=_bytes;
}

void cc6_ec_byte_writeclear(cc6_ec_byte_buffer *_b){
  cc6_celt_free(_b->buf);
}



void cc6_ec_enc_bits(cc6_ec_enc *_this,cc6_ec_uint32 _fl,int _ftb){
  unsigned fl;
  unsigned ft;
  while(_ftb>cc6_EC_UNIT_BITS){
    _ftb-=cc6_EC_UNIT_BITS;
    fl=(unsigned)(_fl>>_ftb)&cc6_EC_UNIT_MASK;
    cc6_ec_encode_bin(_this,fl,fl+1,cc6_EC_UNIT_BITS);
  }
  ft=1<<_ftb;
  fl=(unsigned)_fl&ft-1;
  cc6_ec_encode_bin(_this,fl,fl+1,_ftb);
}

void cc6_ec_enc_uint(cc6_ec_enc *_this,cc6_ec_uint32 _fl,cc6_ec_uint32 _ft){
  unsigned  ft;
  unsigned  fl;
  int       ftb;
  /*In order to optimize cc6_EC_ILOG(), it is undefined for the value 0.*/
  cc6_celt_assert(_ft>1);
  _ft--;
  ftb=cc6_EC_ILOG(_ft);
  if(ftb>cc6_EC_UNIT_BITS){
    ftb-=cc6_EC_UNIT_BITS;
    ft=(_ft>>ftb)+1;
    fl=(unsigned)(_fl>>ftb);
    cc6_ec_encode(_this,fl,fl+1,ft);
    cc6_ec_enc_bits(_this,_fl,ftb);
  } else {
    cc6_ec_encode(_this,_fl,_fl+1,_ft+1);
  }
}

