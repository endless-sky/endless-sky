#ifndef CORE_BUFFERLINE_H
#define CORE_BUFFERLINE_H

#include <array>

#include "alspan.h"

/* Size for temporary storage of buffer data, in floats. Larger values need
 * more memory and are harder on cache, while smaller values may need more
 * iterations for mixing.
 */
constexpr int BufferLineSize{1024};

using FloatBufferLine = std::array<float,BufferLineSize>;
using FloatBufferSpan = al::span<float,BufferLineSize>;

#endif /* CORE_BUFFERLINE_H */
