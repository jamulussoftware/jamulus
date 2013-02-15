/* celt_types.h taken from libogg */
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: cc6_celt_types.h,v 1.2 2013-02-15 19:49:58 corrados Exp $

 ********************************************************************/
/**
   @file celt_types.h
   @brief CELT types
*/
#ifndef cc6__CELT_TYPES_H
#define cc6__CELT_TYPES_H

/* Use the real stdint.h if it's there (taken from Paul Hsieh's pstdint.h) */
#if (defined(__STDC__) && __STDC__ && __STDC_VERSION__ >= 199901L) || (defined(__GNUC__) && (defined(_STDINT_H) || defined(_STDINT_H_)) || defined (HAVE_STDINT_H))
#include <stdint.h>

   typedef int16_t cc6_celt_int16_t;
   typedef uint16_t cc6_celt_uint16_t;
   typedef int32_t cc6_celt_int32_t;
   typedef uint32_t cc6_celt_uint32_t;
   typedef int64_t cc6_celt_int64_t;
   typedef uint64_t cc6_celt_uint64_t;
#elif defined(_WIN32) 

#  if defined(__CYGWIN__)
#    include <_G_config.h>
     typedef _G_int32_t cc6_celt_int32_t;
     typedef _G_uint32_t cc6_celt_uint32_t;
     typedef _G_int16_t cc6_celt_int16_t;
     typedef _G_uint16_t cc6_celt_uint16_t;
     typedef _G_int64_t cc6_celt_int64_t;
     typedef _G_uint64_t cc6_celt_uint64_t;
#  elif defined(__MINGW32__)
     typedef short cc6_celt_int16_t;
     typedef unsigned short cc6_celt_uint16_t;
     typedef int cc6_celt_int32_t;
     typedef unsigned int cc6_celt_uint32_t;
     typedef long long cc6_celt_int64_t;
     typedef unsigned long long cc6_celt_uint64_t;
#  elif defined(__MWERKS__)
     typedef int cc6_celt_int32_t;
     typedef unsigned int cc6_celt_uint32_t;
     typedef short cc6_celt_int16_t;
     typedef unsigned short cc6_celt_uint16_t;
     typedef long long cc6_celt_int64_t;
     typedef unsigned long long cc6_celt_uint64_t;
#  else
     /* MSVC/Borland */
     typedef __int32 cc6_celt_int32_t;
     typedef unsigned __int32 cc6_celt_uint32_t;
     typedef __int16 cc6_celt_int16_t;
     typedef unsigned __int16 cc6_celt_uint16_t;
     typedef __int64 cc6_celt_int64_t;
     typedef unsigned __int64 cc6_celt_uint64_t;
#  endif

#elif defined(__MACOS__)

#  include <sys/types.h>
   typedef SInt16 cc6_celt_int16_t;
   typedef UInt16 cc6_celt_uint16_t;
   typedef SInt32 cc6_celt_int32_t;
   typedef UInt32 cc6_celt_uint32_t;
   typedef SInt64 cc6_celt_int64_t;
   typedef UInt64 cc6_celt_uint64_t;

#elif (defined(__APPLE__) && defined(__MACH__)) /* MacOS X Framework build */

#  include <sys/types.h>
   typedef int16_t cc6_celt_int16_t;
   typedef u_int16_t cc6_celt_uint16_t;
   typedef int32_t cc6_celt_int32_t;
   typedef u_int32_t cc6_celt_uint32_t;
   typedef int64_t cc6_celt_int64_t;
   typedef u_int64_t cc6_celt_uint64_t;

#elif defined(__BEOS__)

   /* Be */
#  include <inttypes.h>
   typedef int16_t cc6_celt_int16_t;
   typedef u_int16_t cc6_celt_uint16_t;
   typedef int32_t cc6_celt_int32_t;
   typedef u_int32_t cc6_celt_uint32_t;
   typedef int64_t cc6_celt_int64_t;
   typedef u_int64_t cc6_celt_uint64_t;

#elif defined (__EMX__)

   /* OS/2 GCC */
   typedef short cc6_celt_int16_t;
   typedef unsigned short cc6_celt_uint16_t;
   typedef int cc6_celt_int32_t;
   typedef unsigned int cc6_celt_uint32_t;
   typedef long long cc6_celt_int64_t;
   typedef unsigned long long cc6_celt_uint64_t;

#elif defined (DJGPP)

   /* DJGPP */
   typedef short cc6_celt_int16_t;
   typedef int cc6_celt_int32_t;
   typedef unsigned int cc6_celt_uint32_t;
   typedef long long cc6_celt_int64_t;
   typedef unsigned long long cc6_celt_uint64_t;

#elif defined(R5900)

   /* PS2 EE */
   typedef int cc6_celt_int32_t;
   typedef unsigned cc6_celt_uint32_t;
   typedef short cc6_celt_int16_t;
   typedef long cc6_celt_int64_t;
   typedef unsigned long cc6_celt_uint64_t;

#elif defined(__SYMBIAN32__)

   /* Symbian GCC */
   typedef signed short cc6_celt_int16_t;
   typedef unsigned short cc6_celt_uint16_t;
   typedef signed int cc6_celt_int32_t;
   typedef unsigned int cc6_celt_uint32_t;
   typedef long long int cc6_celt_int64_t;
   typedef unsigned long long int cc6_celt_uint64_t;

#elif defined(CONFIG_TI_C54X) || defined (CONFIG_TI_C55X)

   typedef short cc6_celt_int16_t;
   typedef unsigned short cc6_celt_uint16_t;
   typedef long cc6_celt_int32_t;
   typedef unsigned long cc6_celt_uint32_t;
   typedef long long cc6_celt_int64_t;
   typedef unsigned long long cc6_celt_uint64_t;

#elif defined(CONFIG_TI_C6X)

   typedef short cc6_celt_int16_t;
   typedef unsigned short cc6_celt_uint16_t;
   typedef int cc6_celt_int32_t;
   typedef unsigned int cc6_celt_uint32_t;
   typedef long long int cc6_celt_int64_t;
   typedef unsigned long long int cc6_celt_uint64_t;

#else

   /* Give up, take a reasonable guess */
   typedef short cc6_celt_int16_t;
   typedef unsigned short cc6_celt_uint16_t;
   typedef int cc6_celt_int32_t;
   typedef unsigned int cc6_celt_uint32_t;
   typedef long long cc6_celt_int64_t;
   typedef unsigned long long cc6_celt_uint64_t;

#endif

#endif  /* cc6__CELT_TYPES_H */
