#ifndef AL_THREADS_H
#define AL_THREADS_H

#if defined(__GNUC__) && defined(__i386__)
/* force_align_arg_pointer is required for proper function arguments aligning
 * when SSE code is used. Some systems (Windows, QNX) do not guarantee our
 * thread functions will be properly aligned on the stack, even though GCC may
 * generate code with the assumption that it is. */
#define FORCE_ALIGN __attribute__((force_align_arg_pointer))
#else
#define FORCE_ALIGN
#endif

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#elif !defined(_WIN32)
#include <semaphore.h>
#endif

void althrd_setname(const char *name);

namespace al {

class semaphore {
#ifdef _WIN32
    using native_type = void*;
#elif defined(__APPLE__)
    using native_type = dispatch_semaphore_t;
#else
    using native_type = sem_t;
#endif
    native_type mSem;

public:
    semaphore(unsigned int initial=0);
    semaphore(const semaphore&) = delete;
    ~semaphore();

    semaphore& operator=(const semaphore&) = delete;

    void post();
    void wait() noexcept;
    bool try_wait() noexcept;
};

} // namespace al

#endif /* AL_THREADS_H */
