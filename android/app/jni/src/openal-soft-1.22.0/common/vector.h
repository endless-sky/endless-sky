#ifndef AL_VECTOR_H
#define AL_VECTOR_H

#include <vector>

#include "almalloc.h"

namespace al {

template<typename T, size_t alignment=alignof(T)>
using vector = std::vector<T, al::allocator<T, alignment>>;

} // namespace al

#endif /* AL_VECTOR_H */
