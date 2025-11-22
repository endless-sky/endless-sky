
#include "config.h"

#include "almalloc.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif


void *al_malloc(size_t alignment, size_t size)
{
    assert((alignment & (alignment-1)) == 0);
    alignment = std::max(alignment, alignof(std::max_align_t));

#if defined(HAVE_POSIX_MEMALIGN)
    void *ret{};
    if(posix_memalign(&ret, alignment, size) == 0)
        return ret;
    return nullptr;
#elif defined(HAVE__ALIGNED_MALLOC)
    return _aligned_malloc(size, alignment);
#else
    size_t total_size{size + alignment-1 + sizeof(void*)};
    void *base{std::malloc(total_size)};
    if(base != nullptr)
    {
        void *aligned_ptr{static_cast<char*>(base) + sizeof(void*)};
        total_size -= sizeof(void*);

        std::align(alignment, size, aligned_ptr, total_size);
        *(static_cast<void**>(aligned_ptr)-1) = base;
        base = aligned_ptr;
    }
    return base;
#endif
}

void *al_calloc(size_t alignment, size_t size)
{
    void *ret{al_malloc(alignment, size)};
    if(ret) std::memset(ret, 0, size);
    return ret;
}

void al_free(void *ptr) noexcept
{
#if defined(HAVE_POSIX_MEMALIGN)
    std::free(ptr);
#elif defined(HAVE__ALIGNED_MALLOC)
    _aligned_free(ptr);
#else
    if(ptr != nullptr)
        std::free(*(static_cast<void**>(ptr) - 1));
#endif
}
