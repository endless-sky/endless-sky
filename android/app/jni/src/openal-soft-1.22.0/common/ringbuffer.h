#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>
#include <memory>
#include <stddef.h>
#include <utility>

#include "albyte.h"
#include "almalloc.h"


/* NOTE: This lockless ringbuffer implementation is copied from JACK, extended
 * to include an element size. Consequently, parameters and return values for a
 * size or count is in 'elements', not bytes. Additionally, it only supports
 * single-consumer/single-provider operation.
 */

struct RingBuffer {
private:
    std::atomic<size_t> mWritePtr{0u};
    std::atomic<size_t> mReadPtr{0u};
    size_t mWriteSize{0u};
    size_t mSizeMask{0u};
    size_t mElemSize{0u};

    al::FlexArray<al::byte, 16> mBuffer;

public:
    struct Data {
        al::byte *buf;
        size_t len;
    };
    using DataPair = std::pair<Data,Data>;


    RingBuffer(const size_t count) : mBuffer{count} { }

    /** Reset the read and write pointers to zero. This is not thread safe. */
    void reset() noexcept;

    /**
     * The non-copying data reader. Returns two ringbuffer data pointers that
     * hold the current readable data. If the readable data is in one segment
     * the second segment has zero length.
     */
    DataPair getReadVector() const noexcept;
    /**
     * The non-copying data writer. Returns two ringbuffer data pointers that
     * hold the current writeable data. If the writeable data is in one segment
     * the second segment has zero length.
     */
    DataPair getWriteVector() const noexcept;

    /**
     * Return the number of elements available for reading. This is the number
     * of elements in front of the read pointer and behind the write pointer.
     */
    size_t readSpace() const noexcept
    {
        const size_t w{mWritePtr.load(std::memory_order_acquire)};
        const size_t r{mReadPtr.load(std::memory_order_acquire)};
        return (w-r) & mSizeMask;
    }

    /**
     * The copying data reader. Copy at most `cnt' elements into `dest'.
     * Returns the actual number of elements copied.
     */
    size_t read(void *dest, size_t cnt) noexcept;
    /**
     * The copying data reader w/o read pointer advance. Copy at most `cnt'
     * elements into `dest'. Returns the actual number of elements copied.
     */
    size_t peek(void *dest, size_t cnt) const noexcept;
    /** Advance the read pointer `cnt' places. */
    void readAdvance(size_t cnt) noexcept
    { mReadPtr.fetch_add(cnt, std::memory_order_acq_rel); }


    /**
     * Return the number of elements available for writing. This is the number
     * of elements in front of the write pointer and behind the read pointer.
     */
    size_t writeSpace() const noexcept
    {
        const size_t w{mWritePtr.load(std::memory_order_acquire)};
        const size_t r{mReadPtr.load(std::memory_order_acquire) + mWriteSize - mSizeMask};
        return (r-w-1) & mSizeMask;
    }

    /**
     * The copying data writer. Copy at most `cnt' elements from `src'. Returns
     * the actual number of elements copied.
     */
    size_t write(const void *src, size_t cnt) noexcept;
    /** Advance the write pointer `cnt' places. */
    void writeAdvance(size_t cnt) noexcept
    { mWritePtr.fetch_add(cnt, std::memory_order_acq_rel); }

    size_t getElemSize() const noexcept { return mElemSize; }

    /**
     * Create a new ringbuffer to hold at least `sz' elements of `elem_sz'
     * bytes. The number of elements is rounded up to the next power of two
     * (even if it is already a power of two, to ensure the requested amount
     * can be written).
     */
    static std::unique_ptr<RingBuffer> Create(size_t sz, size_t elem_sz, int limit_writes);

    DEF_FAM_NEWDEL(RingBuffer, mBuffer)
};
using RingBufferPtr = std::unique_ptr<RingBuffer>;

#endif /* RINGBUFFER_H */
