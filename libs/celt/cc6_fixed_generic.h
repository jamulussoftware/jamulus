/* Copyright (C) 2003-2008 Jean-Marc Valin, CSIRO */
/**
   @file fixed_generic.h
   @brief Generic fixed-point operations
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

#ifndef cc6_FIXED_GENERIC_H
#define cc6_FIXED_GENERIC_H

/** Multiply a 16-bit signed value by a 16-bit unsigned value. The result is a 32-bit signed value */
#define cc6_MULT16_16SU(a,b) ((cc6_celt_word32_t)(cc6_celt_word16_t)(a)*(cc6_celt_word32_t)(cc6_celt_uint16_t)(b))

/** 16x32 multiplication, followed by a 16-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q16(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),16)), cc6_SHR(cc6_MULT16_16SU((a),((b)&0x0000ffff)),16))

/** 16x32 multiplication, followed by a 15-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q15(a,b) cc6_ADD32(cc6_SHL(cc6_MULT16_16((a),cc6_SHR((b),16)),1), cc6_SHR(cc6_MULT16_16SU((a),((b)&0x0000ffff)),15))

/** 32x32 multiplication, followed by a 31-bit shift right. Results fits in 32 bits */
#define cc6_MULT32_32_Q31(a,b) cc6_ADD32(cc6_ADD32(cc6_SHL(cc6_MULT16_16(cc6_SHR((a),16),cc6_SHR((b),16)),1), cc6_SHR(cc6_MULT16_16SU(cc6_SHR((a),16),((b)&0x0000ffff)),15)), cc6_SHR(cc6_MULT16_16SU(cc6_SHR((b),16),((a)&0x0000ffff)),15))

/** 32x32 multiplication, followed by a 32-bit shift right. Results fits in 32 bits */
#define cc6_MULT32_32_Q32(a,b) cc6_ADD32(cc6_ADD32(cc6_MULT16_16(cc6_SHR((a),16),cc6_SHR((b),16)), cc6_SHR(cc6_MULT16_16SU(cc6_SHR((a),16),((b)&0x0000ffff)),16)), cc6_SHR(cc6_MULT16_16SU(cc6_SHR((b),16),((a)&0x0000ffff)),16))

/** Compile-time conversion of float constant to 16-bit value */
#define cc6_QCONST16(x,bits) ((cc6_celt_word16_t)(.5+(x)*(((cc6_celt_word32_t)1)<<(bits))))
/** Compile-time conversion of float constant to 32-bit value */
#define cc6_QCONST32(x,bits) ((cc6_celt_word32_t)(.5+(x)*(((cc6_celt_word32_t)1)<<(bits))))

/** Negate a 16-bit value */
#define cc6_NEG16(x) (-(x))
/** Negate a 32-bit value */
#define cc6_NEG32(x) (-(x))

/** Change a 32-bit value into a 16-bit value. The value is assumed to fit in 16-bit, otherwise the result is undefined */
#define cc6_EXTRACT16(x) ((cc6_celt_word16_t)(x))
/** Change a 16-bit value into a 32-bit value */
#define cc6_EXTEND32(x) ((cc6_celt_word32_t)(x))

/** Arithmetic shift-right of a 16-bit value */
#define cc6_SHR16(a,shift) ((a) >> (shift))
/** Arithmetic shift-left of a 16-bit value */
#define cc6_SHL16(a,shift) ((a) << (shift))
/** Arithmetic shift-right of a 32-bit value */
#define cc6_SHR32(a,shift) ((a) >> (shift))
/** Arithmetic shift-left of a 32-bit value */
#define cc6_SHL32(a,shift) ((cc6_celt_word32_t)(a) << (shift))

/** 16-bit arithmetic shift right with rounding-to-nearest instead of rounding down */
#define cc6_PSHR16(a,shift) (cc6_SHR16((a)+((1<<((shift))>>1)),shift))
/** 32-bit arithmetic shift right with rounding-to-nearest instead of rounding down */
#define cc6_PSHR32(a,shift) (cc6_SHR32((a)+((cc6_EXTEND32(1)<<((shift))>>1)),shift))
/** 32-bit arithmetic shift right where the argument can be negative */
#define cc6_VSHR32(a, shift) (((shift)>0) ? cc6_SHR32(a, shift) : cc6_SHL32(a, -(shift)))

/** Saturates 16-bit value to +/- a */
#define cc6_SATURATE16(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))
/** Saturates 32-bit value to +/- a */
#define cc6_SATURATE32(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

/** "RAW" macros, should not be used outside of this header file */
#define cc6_SHR(a,shift) ((a) >> (shift))
#define cc6_SHL(a,shift) ((cc6_celt_word32_t)(a) << (shift))
#define cc6_PSHR(a,shift) (cc6_SHR((a)+((cc6_EXTEND32(1)<<((shift))>>1)),shift))
#define cc6_SATURATE(x,a) (((x)>(a) ? (a) : (x)<-(a) ? -(a) : (x)))

/** Shift by a and round-to-neareast 32-bit value. Result is a 16-bit value */
#define cc6_ROUND16(x,a) (cc6_EXTRACT16(cc6_PSHR32((x),(a))))
/** Divide by two */
#define cc6_HALF32(x)  (cc6_SHR32(x,1))

/** Add two 16-bit values */
#define cc6_ADD16(a,b) ((cc6_celt_word16_t)((cc6_celt_word16_t)(a)+(cc6_celt_word16_t)(b)))
/** Subtract two 16-bit values */
#define cc6_SUB16(a,b) ((cc6_celt_word16_t)(a)-(cc6_celt_word16_t)(b))
/** Add two 32-bit values */
#define cc6_ADD32(a,b) ((cc6_celt_word32_t)(a)+(cc6_celt_word32_t)(b))
/** Subtract two 32-bit values */
#define cc6_SUB32(a,b) ((cc6_celt_word32_t)(a)-(cc6_celt_word32_t)(b))


/** 16x16 multiplication where the result fits in 16 bits */
#define cc6_MULT16_16_16(a,b)     ((((cc6_celt_word16_t)(a))*((cc6_celt_word16_t)(b))))

/* (cc6_celt_word32_t)(cc6_celt_word16_t) gives TI compiler a hint that it's 16x16->32 multiply */
/** 16x16 multiplication where the result fits in 32 bits */
#define cc6_MULT16_16(a,b)     (((cc6_celt_word32_t)(cc6_celt_word16_t)(a))*((cc6_celt_word32_t)(cc6_celt_word16_t)(b)))

/** 16x16 multiply-add where the result fits in 32 bits */
#define cc6_MAC16_16(c,a,b) (cc6_ADD32((c),cc6_MULT16_16((a),(b))))
/** 16x32 multiplication, followed by a 12-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q12(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),12)), cc6_SHR(cc6_MULT16_16((a),((b)&0x00000fff)),12))
/** 16x32 multiplication, followed by a 13-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q13(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),13)), cc6_SHR(cc6_MULT16_16((a),((b)&0x00001fff)),13))
/** 16x32 multiplication, followed by a 14-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q14(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),14)), cc6_SHR(cc6_MULT16_16((a),((b)&0x00003fff)),14))

/** 16x32 multiplication, followed by an 11-bit shift right. Results fits in 32 bits */
#define cc6_MULT16_32_Q11(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),11)), cc6_SHR(cc6_MULT16_16((a),((b)&0x000007ff)),11))
/** 16x32 multiply-add, followed by an 11-bit shift right. Results fits in 32 bits */
#define cc6_MAC16_32_Q11(c,a,b) cc6_ADD32(c,cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),11)), cc6_SHR(cc6_MULT16_16((a),((b)&0x000007ff)),11)))

/** 16x32 multiplication, followed by a 15-bit shift right (round-to-nearest). Results fits in 32 bits */
#define cc6_MULT16_32_P15(a,b) cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),15)), cc6_PSHR(cc6_MULT16_16((a),((b)&0x00007fff)),15))
/** 16x32 multiply-add, followed by a 15-bit shift right. Results fits in 32 bits */
#define cc6_MAC16_32_Q15(c,a,b) cc6_ADD32(c,cc6_ADD32(cc6_MULT16_16((a),cc6_SHR((b),15)), cc6_SHR(cc6_MULT16_16((a),((b)&0x00007fff)),15)))


#define cc6_MAC16_16_Q11(c,a,b)     (cc6_ADD32((c),cc6_SHR(cc6_MULT16_16((a),(b)),11)))
#define cc6_MAC16_16_Q13(c,a,b)     (cc6_ADD32((c),cc6_SHR(cc6_MULT16_16((a),(b)),13)))
#define cc6_MAC16_16_P13(c,a,b)     (cc6_ADD32((c),cc6_SHR(cc6_ADD32(4096,cc6_MULT16_16((a),(b))),13)))

#define cc6_MULT16_16_Q11_32(a,b) (cc6_SHR(cc6_MULT16_16((a),(b)),11))
#define cc6_MULT16_16_Q13(a,b) (cc6_SHR(cc6_MULT16_16((a),(b)),13))
#define cc6_MULT16_16_Q14(a,b) (cc6_SHR(cc6_MULT16_16((a),(b)),14))
#define cc6_MULT16_16_Q15(a,b) (cc6_SHR(cc6_MULT16_16((a),(b)),15))

#define cc6_MULT16_16_P13(a,b) (cc6_SHR(cc6_ADD32(4096,cc6_MULT16_16((a),(b))),13))
#define cc6_MULT16_16_P14(a,b) (cc6_SHR(cc6_ADD32(8192,cc6_MULT16_16((a),(b))),14))
#define cc6_MULT16_16_P15(a,b) (cc6_SHR(cc6_ADD32(16384,cc6_MULT16_16((a),(b))),15))

/** Divide a 32-bit value by a 16-bit value. Result fits in 16 bits */
#define cc6_DIV32_16(a,b) ((cc6_celt_word16_t)(((cc6_celt_word32_t)(a))/((cc6_celt_word16_t)(b))))
/** Divide a 32-bit value by a 16-bit value and round to nearest. Result fits in 16 bits */
#define cc6_PDIV32_16(a,b) ((cc6_celt_word16_t)(((cc6_celt_word32_t)(a)+((cc6_celt_word16_t)(b)>>1))/((cc6_celt_word16_t)(b))))
/** Divide a 32-bit value by a 32-bit value. Result fits in 32 bits */
#define cc6_DIV32(a,b) (((cc6_celt_word32_t)(a))/((cc6_celt_word32_t)(b)))
/** Divide a 32-bit value by a 32-bit value and round to nearest. Result fits in 32 bits */
#define cc6_PDIV32(a,b) (((cc6_celt_word32_t)(a)+((cc6_celt_word16_t)(b)>>1))/((cc6_celt_word32_t)(b)))

#endif
