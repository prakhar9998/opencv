/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Copyright (C) 2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef OPENCV_CORE_FAST_MATH_HPP
#define OPENCV_CORE_FAST_MATH_HPP

#include "opencv2/core/cvdef.h"

#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ \
    && defined __SSE2__ && !defined __APPLE__)) && !defined(__CUDACC__)
#include <emmintrin.h>
#endif


//! @addtogroup core_utils
//! @{

/****************************************************************************************\
*                                      fast math                                         *
\****************************************************************************************/

#ifdef __cplusplus
#  include <cmath>
#else
#  ifdef __BORLANDC__
#    include <fastmath.h>
#  else
#    include <math.h>
#  endif
#endif

#ifdef HAVE_TEGRA_OPTIMIZATION
#  include "tegra_round.hpp"
#endif

#if defined __PPC64__ && defined __GNUC__ && defined _ARCH_PWR8 && !defined (__CUDACC__)
#  include <altivec.h>
#endif

#if ((defined _MSC_VER && defined _M_ARM) || defined CV_ICC || \
        defined __GNUC__) && defined HAVE_TEGRA_OPTIMIZATION
    #define CV_INLINE_ROUND_DBL(value) TEGRA_ROUND_DBL(value);
    #define CV_INLINE_ROUND_FLT(value) TEGRA_ROUND_FLT(value);
#elif defined __GNUC__ && defined __arm__ && (defined __ARM_PCS_VFP || defined __ARM_VFPV3__ || defined __ARM_NEON__) && !defined __SOFTFP__ && !defined(__CUDACC__)
    // 1. general scheme
    #define ARM_ROUND(_value, _asm_string) \
        int res; \
        float temp; \
        CV_UNUSED(temp); \
        __asm__(_asm_string : [res] "=r" (res), [temp] "=w" (temp) : [value] "w" (_value)); \
        return res
    // 2. version for double
    #ifdef __clang__
        #define CV_INLINE_ROUND_DBL(value) ARM_ROUND(value, "vcvtr.s32.f64 %[temp], %[value] \n vmov %[res], %[temp]")
    #else
        #define CV_INLINE_ROUND_DBL(value) ARM_ROUND(value, "vcvtr.s32.f64 %[temp], %P[value] \n vmov %[res], %[temp]")
    #endif
    // 3. version for float
    #define CV_INLINE_ROUND_FLT(value) ARM_ROUND(value, "vcvtr.s32.f32 %[temp], %[value]\n vmov %[res], %[temp]")
#elif defined __PPC64__ && defined __GNUC__ && defined _ARCH_PWR8 && !defined (__CUDACC__)
    // P8 and newer machines can convert fp32/64 to int quickly.
    #define CV_INLINE_ROUND_DBL(value) \
        int out; \
        double temp; \
        __asm__( "fctiw %[temp],%[in]\n\tmffprwz %[out],%[temp]\n\t" : [out] "=r" (out), [temp] "=d" (temp) : [in] "d" ((double)(value)) : ); \
        return out;

    // FP32 also works with FP64 routine above
    #define CV_INLINE_ROUND_FLT(value) CV_INLINE_ROUND_DBL(value)

    #ifdef _ARCH_PWR9
        #define CV_INLINE_ISINF_DBL(value) return scalar_test_data_class(value, 0x30);
        #define CV_INLINE_ISNAN_DBL(value) return scalar_test_data_class(value, 0x40);
        #define CV_INLINE_ISINF_FLT(value) CV_INLINE_ISINF_DBL(value)
        #define CV_INLINE_ISNAN_FLT(value) CV_INLINE_ISNAN_DBL(value)
    #endif
#elif defined CV_ICC || defined __GNUC__
    #define CV_INLINE_ROUND_DBL(value) return (int)(lrint(value));
    #define CV_INLINE_ROUND_FLT(value) return (int)(lrintf(value));
#endif

#if defined __PPC64__ && !defined OPENCV_USE_FASTMATH_GCC_BUILTINS
    /* Let GCC inline C math functions when available. Dedicated hardware is available to
       round and covert FP values. */
    #define OPENCV_USE_FASTMATH_GCC_BUILTINS
#endif

/* Enable GCC builtin math functions if possible, desired, and available.
   Note, not all math functions inline equally. E.g lrint will not inline
   without the -fno-math-errno option. */
#if defined OPENCV_USE_FASTMATH_GCC_BUILTINS && defined __GNUC__ && !defined __clang__ && !defined (__CUDACC__)
    #define _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS
#endif

/* Allow overrides for some functions which may benefit from tuning. Likewise,
   note that isinf is not used as the return value is signed. */
#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS && !defined CV_INLINE_ISNAN_DBL
    #define CV_INLINE_ISNAN_DBL(value) return __builtin_isnan(value);
#endif

#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS && !defined CV_INLINE_ISNAN_FLT
    #define CV_INLINE_ISNAN_FLT(value) return __builtin_isnanf(value);
#endif

/** @brief Rounds floating-point number to the nearest integer

 @param value floating-point number. If the value is outside of INT_MIN ... INT_MAX range, the
 result is not defined.
 */
CV_INLINE int
cvRound( double value )
{
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ \
    && defined __SSE2__ && !defined __APPLE__) || CV_SSE2) && !defined(__CUDACC__)
    __m128d t = _mm_set_sd( value );
    return _mm_cvtsd_si32(t);
#elif defined _MSC_VER && defined _M_IX86
    int t;
    __asm
    {
        fld value;
        fistp t;
    }
    return t;
#elif defined CV_INLINE_ROUND_DBL
    CV_INLINE_ROUND_DBL(value);
#else
    /* it's ok if round does not comply with IEEE754 standard;
       the tests should allow +/-1 difference when the tested functions use round */
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
#endif
}


/** @brief Rounds floating-point number to the nearest integer not larger than the original.

 The function computes an integer i such that:
 \f[i \le \texttt{value} < i+1\f]
 @param value floating-point number. If the value is outside of INT_MIN ... INT_MAX range, the
 result is not defined.
 */
CV_INLINE int cvFloor( double value )
{
#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS
    return __builtin_floor(value);
#else
    int i = (int)value;
    return i - (i > value);
#endif
}

/** @brief Rounds floating-point number to the nearest integer not smaller than the original.

 The function computes an integer i such that:
 \f[i \le \texttt{value} < i+1\f]
 @param value floating-point number. If the value is outside of INT_MIN ... INT_MAX range, the
 result is not defined.
 */
CV_INLINE int cvCeil( double value )
{
#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS
    return __builtin_ceil(value);
#else
    int i = (int)value;
    return i + (i < value);
#endif
}

/** @brief Determines if the argument is Not A Number.

 @param value The input floating-point value

 The function returns 1 if the argument is Not A Number (as defined by IEEE754 standard), 0
 otherwise. */
CV_INLINE int cvIsNaN( double value )
{
#if defined CV_INLINE_ISNAN_DBL
    CV_INLINE_ISNAN_DBL(value);
#else
    Cv64suf ieee754;
    ieee754.f = value;
    return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) +
           ((unsigned)ieee754.u != 0) > 0x7ff00000;
#endif
}

/** @brief Determines if the argument is Infinity.

 @param value The input floating-point value

 The function returns 1 if the argument is a plus or minus infinity (as defined by IEEE754 standard)
 and 0 otherwise. */
CV_INLINE int cvIsInf( double value )
{
#if defined CV_INLINE_ISINF_DBL
    CV_INLINE_ISINF_DBL(value);
#else
    Cv64suf ieee754;
    ieee754.f = value;
    return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 &&
            (unsigned)ieee754.u == 0;
#endif
}

#ifdef __cplusplus

/** @overload */
CV_INLINE int cvRound(float value)
{
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ \
    && defined __SSE2__ && !defined __APPLE__) || CV_SSE2) && !defined(__CUDACC__)
    __m128 t = _mm_set_ss( value );
    return _mm_cvtss_si32(t);
#elif defined _MSC_VER && defined _M_IX86
    int t;
    __asm
    {
        fld value;
        fistp t;
    }
    return t;
#elif defined CV_INLINE_ROUND_FLT
    CV_INLINE_ROUND_FLT(value);
#else
    /* it's ok if round does not comply with IEEE754 standard;
     the tests should allow +/-1 difference when the tested functions use round */
    return (int)(value + (value >= 0 ? 0.5f : -0.5f));
#endif
}

/** @overload */
CV_INLINE int cvRound( int value )
{
    return value;
}

/** @overload */
CV_INLINE int cvFloor( float value )
{
#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS
    return __builtin_floorf(value);
#else
    int i = (int)value;
    return i - (i > value);
#endif
}

/** @overload */
CV_INLINE int cvFloor( int value )
{
    return value;
}

/** @overload */
CV_INLINE int cvCeil( float value )
{
#if defined _OPENCV_FASTMATH_ENABLE_GCC_MATH_BUILTINS
    return __builtin_ceilf(value);
#else
    int i = (int)value;
    return i + (i < value);
#endif
}

/** @overload */
CV_INLINE int cvCeil( int value )
{
    return value;
}

/** @overload */
CV_INLINE int cvIsNaN( float value )
{
#if defined CV_INLINE_ISNAN_FLT
    CV_INLINE_ISNAN_FLT(value);
#else
    Cv32suf ieee754;
    ieee754.f = value;
    return (ieee754.u & 0x7fffffff) > 0x7f800000;
#endif
}

/** @overload */
CV_INLINE int cvIsInf( float value )
{
#if defined CV_INLINE_ISINF_FLT
    CV_INLINE_ISINF_FLT(value);
#else
    Cv32suf ieee754;
    ieee754.f = value;
    return (ieee754.u & 0x7fffffff) == 0x7f800000;
#endif
}

#endif // __cplusplus

//! @} core_utils

#endif
