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

#include "NvFoundation.h"

#ifndef NVP_PLATFORM_H__
#define NVP_PLATFORM_H__

#define NVP_RESTRICT      NV_RESTRICT
#define NVP_INLINE        NV_INLINE

#define NVP_ALIGN_V       NV_ALIGN
#define NVP_ALIGN_BEGIN   NV_ALIGN_PREFIX
#define NVP_ALIGN_END     NV_ALIGN_SUFFIX

#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ >= 3 && __GNUC_MINOR__ >= 4))

  #define NVP_NOOP(...)
  #define NVP_BARRIER()       __sync_synchronize()

/*
// maybe better than __sync_synchronize?
#if defined(__i386__ ) || defined(__x64__)
#define NVP_BARRIER()  __asm__ __volatile__ ("mfence" ::: "memory")
#endif

#if defined(__arm__)
#define NVP_BARRIER() __asm__ __volatile__ ("dmb" :::"memory")
#endif
*/
  
#elif defined(__MSC__) || defined(_MSC_VER)

  #include <emmintrin.h>

  #pragma warning(disable:4142) // redefinition of same type
  #if (_MSC_VER >= 1400)      // VC8+
    #pragma warning(disable : 4996)    // Either disable all deprecation warnings,
  #endif   // VC8+

  #define NVP_NOOP            __noop
  #define NVP_BARRIER()       _mm_mfence()

#else
  #error "compiler unkown"
#endif

#endif
