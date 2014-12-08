/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

#ifndef NVP_PLATFORM_H__
#define NVP_PLATFORM_H__

#if defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 4
  #define NVP_COMPILER_GCC
  #define NVP_NOOP(...)
  #define NVP_INLINE          inline
  #define NVP_STACKALLOC      alloca
  #define NVP_STACKALLOC16    ((void *)((((size_t)alloca( (x)+15 )) + 15) & ~15))
  #define NVP_ALIGN_V(x,a)    x __attribute__((aligned (a)))
  #define NVP_ALIGN_BEGIN(a)  
  #define NVP_ALIGN_END(a)    __attribute__((aligned (a)))
  #define NVP_FASTCALL        __attribute__((fastcall))
  #define NVP_RESTRICT        __restrict__
  #define NVP_ASSUME          NVP_NOOP
  #define NVP_BARRIER()       __sync_synchronize()
  
#elif defined(__MSC__) || defined(_MSC_VER)

  #include <emmintrin.h>

  #pragma warning(disable:4142) // redefinition of same type
  #if (_MSC_VER >= 1400)      // VC8+
    #pragma warning(disable : 4996)    // Either disable all deprecation warnings,
  #endif   // VC8+

  #define NVP_COMPILER_MSC
  #define NVP_NOOP            __noop
  #define NVP_INLINE          __forceinline
  #define NVP_STACKALLOC      _alloca
  #define NVP_STACKALLOC16    ((void *)((((size_t)_alloca( (x)+15 )) + 15) & ~15))
  #define NVP_ALIGN_V(x,a)    __declspec(align(a)) x
  #define NVP_ALIGN_BEGIN(a)  __declspec(align(a))
  #define NVP_ALIGN_END(a)  
  #define NVP_FASTCALL        __fastcall
  #define NVP_RESTRICT        __restrict
  #define NVP_ASSUME          __assume
  #define NVP_BARRIER()       _mm_mfence()

#else
  #error "compiler unkown"
#endif

#endif