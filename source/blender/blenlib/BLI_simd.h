/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/** \file
 * \ingroup bli
 *
 * SIMD instruction support.
 */

#if defined(__ARM_NEON) && defined(WITH_SSE2NEON)
/* SSE/SSE2 emulation on ARM Neon. Match SSE precision. */
#  if !defined(SSE2NEON_PRECISE_MINMAX)
#    define SSE2NEON_PRECISE_MINMAX 1
#  endif
#  if !defined(SSE2NEON_PRECISE_DIV)
#    define SSE2NEON_PRECISE_DIV 1
#  endif
#  if !defined(SSE2NEON_PRECISE_SQRT)
#    define SSE2NEON_PRECISE_SQRT 1
#  endif
#  include <sse2neon.h>
#  define BLI_HAVE_SSE2 1
#elif defined(__SSE2__)
/* Native SSE2 on Intel/AMD. */
#  include <emmintrin.h>
#  define BLI_HAVE_SSE2 1
#else
#  define BLI_HAVE_SSE2 0
#endif

#if defined(__SSE4_1__) || (defined(__ARM_NEON) && defined(WITH_SSE2NEON))
#  define BLI_HAVE_SSE4 1
#else
#  define BLI_HAVE_SSE4 0
#endif
