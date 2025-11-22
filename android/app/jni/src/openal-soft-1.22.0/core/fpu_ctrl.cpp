
#include "config.h"

#include "fpu_ctrl.h"

#ifdef HAVE_INTRIN_H
#include <intrin.h>
#endif
#ifdef HAVE_SSE_INTRINSICS
#include <emmintrin.h>
#ifndef _MM_DENORMALS_ZERO_MASK
/* Some headers seem to be missing these? */
#define _MM_DENORMALS_ZERO_MASK 0x0040u
#define _MM_DENORMALS_ZERO_ON 0x0040u
#endif
#endif

#include "cpu_caps.h"


void FPUCtl::enter() noexcept
{
    if(this->in_mode) return;

#if defined(HAVE_SSE_INTRINSICS)
    this->sse_state = _mm_getcsr();
    unsigned int sseState{this->sse_state};
    sseState &= ~(_MM_FLUSH_ZERO_MASK | _MM_DENORMALS_ZERO_MASK);
    sseState |= _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON;
    _mm_setcsr(sseState);

#elif defined(__GNUC__) && defined(HAVE_SSE)

    if((CPUCapFlags&CPU_CAP_SSE))
    {
        __asm__ __volatile__("stmxcsr %0" : "=m" (*&this->sse_state));
        unsigned int sseState{this->sse_state};
        sseState |= 0x8000; /* set flush-to-zero */
        if((CPUCapFlags&CPU_CAP_SSE2))
            sseState |= 0x0040; /* set denormals-are-zero */
        __asm__ __volatile__("ldmxcsr %0" : : "m" (*&sseState));
    }
#endif

    this->in_mode = true;
}

void FPUCtl::leave() noexcept
{
    if(!this->in_mode) return;

#if defined(HAVE_SSE_INTRINSICS)
    _mm_setcsr(this->sse_state);

#elif defined(__GNUC__) && defined(HAVE_SSE)

    if((CPUCapFlags&CPU_CAP_SSE))
        __asm__ __volatile__("ldmxcsr %0" : : "m" (*&this->sse_state));
#endif
    this->in_mode = false;
}
