/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include "ringbuffer.h"

#include <algorithm>
#include <climits>
#include <stdexcept>

#include "almalloc.h"


RingBufferPtr RingBuffer::Create(size_t sz, size_t elem_sz, int limit_writes)
{
    size_t power_of_two{0u};
    if(sz > 0)
    {
        power_of_two = sz;
        power_of_two |= power_of_two>>1;
        power_of_two |= power_of_two>>2;
        power_of_two |= power_of_two>>4;
        power_of_two |= power_of_two>>8;
        power_of_two |= power_of_two>>16;
#if SIZE_MAX > UINT_MAX
        power_of_two |= power_of_two>>32;
#endif
    }
    ++power_of_two;
    if(power_of_two <= sz || power_of_two > std::numeric_limits<size_t>::max()/elem_sz)
        throw std::overflow_error{"Ring buffer size overflow"};

    const size_t bufbytes{power_of_two * elem_sz};
    RingBufferPtr rb{new(FamCount(bufbytes)) RingBuffer{bufbytes}};
    rb->mWriteSize = limit_writes ? sz : (power_of_two-1);
    rb->mSizeMask = power_of_two - 1;
    rb->mElemSize = elem_sz;

    return rb;
}

void RingBuffer::reset() noexcept
{
    mWritePtr.store(0, std::memory_order_relaxed);
    mReadPtr.store(0, std::memory_order_relaxed);
    std::fill_n(mBuffer.begin(), (mSizeMask+1)*mElemSize, al::byte{});
}


size_t RingBuffer::read(void *dest, size_t cnt) noexcept
{
    const size_t free_cnt{readSpace()};
    if(free_cnt == 0) return 0;

    const size_t to_read{std::min(cnt, free_cnt)};
    size_t read_ptr{mReadPtr.load(std::memory_order_relaxed) & mSizeMask};

    size_t n1, n2;
    const size_t cnt2{read_ptr + to_read};
    if(cnt2 > mSizeMask+1)
    {
        n1 = mSizeMask+1 - read_ptr;
        n2 = cnt2 & mSizeMask;
    }
    else
    {
        n1 = to_read;
        n2 = 0;
    }

    auto outiter = std::copy_n(mBuffer.begin() + read_ptr*mElemSize, n1*mElemSize,
        static_cast<al::byte*>(dest));
    read_ptr += n1;
    if(n2 > 0)
    {
        std::copy_n(mBuffer.begin(), n2*mElemSize, outiter);
        read_ptr += n2;
    }
    mReadPtr.store(read_ptr, std::memory_order_release);
    return to_read;
}

size_t RingBuffer::peek(void *dest, size_t cnt) const noexcept
{
    const size_t free_cnt{readSpace()};
    if(free_cnt == 0) return 0;

    const size_t to_read{std::min(cnt, free_cnt)};
    size_t read_ptr{mReadPtr.load(std::memory_order_relaxed) & mSizeMask};

    size_t n1, n2;
    const size_t cnt2{read_ptr + to_read};
    if(cnt2 > mSizeMask+1)
    {
        n1 = mSizeMask+1 - read_ptr;
        n2 = cnt2 & mSizeMask;
    }
    else
    {
        n1 = to_read;
        n2 = 0;
    }

    auto outiter = std::copy_n(mBuffer.begin() + read_ptr*mElemSize, n1*mElemSize,
        static_cast<al::byte*>(dest));
    if(n2 > 0)
        std::copy_n(mBuffer.begin(), n2*mElemSize, outiter);
    return to_read;
}

size_t RingBuffer::write(const void *src, size_t cnt) noexcept
{
    const size_t free_cnt{writeSpace()};
    if(free_cnt == 0) return 0;

    const size_t to_write{std::min(cnt, free_cnt)};
    size_t write_ptr{mWritePtr.load(std::memory_order_relaxed) & mSizeMask};

    size_t n1, n2;
    const size_t cnt2{write_ptr + to_write};
    if(cnt2 > mSizeMask+1)
    {
        n1 = mSizeMask+1 - write_ptr;
        n2 = cnt2 & mSizeMask;
    }
    else
    {
        n1 = to_write;
        n2 = 0;
    }

    auto srcbytes = static_cast<const al::byte*>(src);
    std::copy_n(srcbytes, n1*mElemSize, mBuffer.begin() + write_ptr*mElemSize);
    write_ptr += n1;
    if(n2 > 0)
    {
        std::copy_n(srcbytes + n1*mElemSize, n2*mElemSize, mBuffer.begin());
        write_ptr += n2;
    }
    mWritePtr.store(write_ptr, std::memory_order_release);
    return to_write;
}


auto RingBuffer::getReadVector() const noexcept -> DataPair
{
    DataPair ret;

    size_t w{mWritePtr.load(std::memory_order_acquire)};
    size_t r{mReadPtr.load(std::memory_order_acquire)};
    w &= mSizeMask;
    r &= mSizeMask;
    const size_t free_cnt{(w-r) & mSizeMask};

    const size_t cnt2{r + free_cnt};
    if(cnt2 > mSizeMask+1)
    {
        /* Two part vector: the rest of the buffer after the current read ptr,
         * plus some from the start of the buffer. */
        ret.first.buf = const_cast<al::byte*>(mBuffer.data() + r*mElemSize);
        ret.first.len = mSizeMask+1 - r;
        ret.second.buf = const_cast<al::byte*>(mBuffer.data());
        ret.second.len = cnt2 & mSizeMask;
    }
    else
    {
        /* Single part vector: just the rest of the buffer */
        ret.first.buf = const_cast<al::byte*>(mBuffer.data() + r*mElemSize);
        ret.first.len = free_cnt;
        ret.second.buf = nullptr;
        ret.second.len = 0;
    }

    return ret;
}

auto RingBuffer::getWriteVector() const noexcept -> DataPair
{
    DataPair ret;

    size_t w{mWritePtr.load(std::memory_order_acquire)};
    size_t r{mReadPtr.load(std::memory_order_acquire) + mWriteSize - mSizeMask};
    w &= mSizeMask;
    r &= mSizeMask;
    const size_t free_cnt{(r-w-1) & mSizeMask};

    const size_t cnt2{w + free_cnt};
    if(cnt2 > mSizeMask+1)
    {
        /* Two part vector: the rest of the buffer after the current write ptr,
         * plus some from the start of the buffer. */
        ret.first.buf = const_cast<al::byte*>(mBuffer.data() + w*mElemSize);
        ret.first.len = mSizeMask+1 - w;
        ret.second.buf = const_cast<al::byte*>(mBuffer.data());
        ret.second.len = cnt2 & mSizeMask;
    }
    else
    {
        ret.first.buf = const_cast<al::byte*>(mBuffer.data() + w*mElemSize);
        ret.first.len = free_cnt;
        ret.second.buf = nullptr;
        ret.second.len = 0;
    }

    return ret;
}
