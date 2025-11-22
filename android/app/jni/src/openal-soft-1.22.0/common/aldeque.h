#ifndef ALDEQUE_H
#define ALDEQUE_H

#include <deque>

#include "almalloc.h"


namespace al {

template<typename T>
using deque = std::deque<T, al::allocator<T>>;

} // namespace al

#endif /* ALDEQUE_H */
