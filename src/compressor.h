/*
   ZynAddSubFX - a software synthesizer

   Compressor.h - simple audio compressor macros
   Copyright (C) 2016 Hans Petter Selasky

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once

static inline bool
floatIsValid(const float x)
{
    const float __r = x * 0.0f;
    return (__r == 0.0f || __r == -0.0f);
}

#define	stereoCompressor(div,pv,l,r) do {	\
  /*						\
   * Don't max the output range to avoid	\
   * overflowing sample rate conversion and	\
   * equalizer filters in the DSP's output	\
   * path. Keep one 10th, 1dB, reserved.	\
   */						\
  static constexpr float __limit =		\
      1.0f - (1.0f / 10.0f);			\
  float __peak;					\
						\
  /* sanity checks */				\
  __peak = (pv);				\
  if (!floatIsValid(__peak))			\
	__peak = 0.0;				\
  if (!floatIsValid(l))				\
	(l) = 0.0;				\
  if (!floatIsValid(r))				\
	(r) = 0.0;				\
  /* compute maximum */				\
  if ((l) < -__peak)				\
    __peak = -(l);				\
  else if ((l) > __peak)			\
    __peak = (l);				\
  if ((r) < -__peak)				\
    __peak = -(r);				\
  else if ((r) > __peak)			\
    __peak = (r);				\
  /* compressor */				\
  if (__peak > __limit) {			\
    (l) /= __peak;				\
    (r) /= __peak;				\
    (l) *= __limit;				\
    (r) *= __limit;				\
    __peak -= __peak / (div);			\
  }						\
  (pv) = __peak;				\
} while (0)

#define	monoCompressor(div,pv,l) do {		\
  /*						\
   * Don't max the output range to avoid	\
   * overflowing sample rate conversion and	\
   * equalizer filters in the DSP's output	\
   * path. Keep one 10th, 1dB, reserved.	\
   */						\
  static constexpr float __limit =		\
      1.0f - (1.0f / 10.0f);			\
  float __peak;					\
						\
  /* sanity checks */				\
  __peak = (pv);				\
  if (!floatIsValid(__peak))			\
	__peak = 0.0;				\
  if (!floatIsValid(l))				\
	(l) = 0.0;				\
  /* compute maximum */				\
  if ((l) < -__peak)				\
    __peak = -(l);				\
  else if ((l) > __peak)			\
    __peak = (l);				\
  /* compressor */				\
  if (__peak > __limit) {			\
    (l) /= __peak;				\
    (l) *= __limit;				\
    __peak -= __peak / (div);			\
  }						\
  (pv) = __peak;				\
} while (0)

