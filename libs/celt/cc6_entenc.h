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

#if !defined(cc6__entenc_H)
# define cc6__entenc_H (1)
# include <stddef.h>
# include "cc6_entcode.h"



typedef struct cc6_ec_enc cc6_ec_enc;



/*The entropy encoder.*/
struct cc6_ec_enc{
   /*Buffered output.*/
   cc6_ec_byte_buffer *buf;
   /*A buffered output symbol, awaiting carry propagation.*/
   int             rem;
   /*Number of extra carry propagating symbols.*/
   size_t          ext;
   /*The number of values in the current range.*/
   cc6_ec_uint32       rng;
   /*The low end of the current range (inclusive).*/
   cc6_ec_uint32       low;
};


/*Initializes the encoder.
  _buf: The buffer to store output bytes in.
        This must have already been initialized for writing and reset.*/
void cc6_ec_enc_init(cc6_ec_enc *_this,cc6_ec_byte_buffer *_buf);
/*Encodes a symbol given its frequency information.
  The frequency information must be discernable by the decoder, assuming it
   has read only the previous symbols from the stream.
  It is allowable to change the frequency information, or even the entire
   source alphabet, so long as the decoder can tell from the context of the
   previously encoded information that it is supposed to do so as well.
  _fl: The cumulative frequency of all symbols that come before the one to be
        encoded.
  _fh: The cumulative frequency of all symbols up to and including the one to
        be encoded.
       Together with _fl, this defines the range [_fl,_fh) in which the
        decoded value will fall.
  _ft: The sum of the frequencies of all the symbols*/
void cc6_ec_encode(cc6_ec_enc *_this,unsigned _fl,unsigned _fh,unsigned _ft);
void cc6_ec_encode_bin(cc6_ec_enc *_this,unsigned _fl,unsigned _fh,unsigned bits);
/*Encodes a sequence of raw bits in the stream.
  _fl:  The bits to encode.
  _ftb: The number of bits to encode.
        This must be at least one, and no more than 32.*/
void cc6_ec_enc_bits(cc6_ec_enc *_this,cc6_ec_uint32 _fl,int _ftb);
/*Encodes a sequence of raw bits in the stream.
  _fl:  The bits to encode.
  _ftb: The number of bits to encode.
        This must be at least one, and no more than 64.*/
void cc6_ec_enc_bits64(cc6_ec_enc *_this,cc6_ec_uint64 _fl,int _ftb);
/*Encodes a raw unsigned integer in the stream.
  _fl: The integer to encode.
  _ft: The number of integers that can be encoded (one more than the max).
       This must be at least one, and no more than 2**32-1.*/
void cc6_ec_enc_uint(cc6_ec_enc *_this,cc6_ec_uint32 _fl,cc6_ec_uint32 _ft);
/*Encodes a raw unsigned integer in the stream.
  _fl: The integer to encode.
  _ft: The number of integers that can be encoded (one more than the max).
       This must be at least one, and no more than 2**64-1.*/
void cc6_ec_enc_uint64(cc6_ec_enc *_this,cc6_ec_uint64 _fl,cc6_ec_uint64 _ft);

/*Returns the number of bits "used" by the encoded symbols so far.
  The actual number of bits may be larger, due to rounding to whole bytes, or
   smaller, due to trailing zeros that can be stripped, so this is not an
   estimate of the true packet size.
  This same number can be computed by the decoder, and is suitable for making
   coding decisions.
  _b: The number of extra bits of precision to include.
      At most 16 will be accurate.
  Return: The number of bits scaled by 2**_b.
          This will always be slightly larger than the exact value (e.g., all
           rounding error is in the positive direction).*/
long cc6_ec_enc_tell(cc6_ec_enc *_this,int _b);

/*Indicates that there are no more symbols to encode.
  All reamining output bytes are flushed to the output buffer.
  cc6_ec_enc_init() must be called before the encoder can be used again.*/
void cc6_ec_enc_done(cc6_ec_enc *_this);

#endif
