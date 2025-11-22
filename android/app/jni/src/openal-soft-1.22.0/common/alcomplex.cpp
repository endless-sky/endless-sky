
#include "config.h"

#include "alcomplex.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>

#include "albit.h"
#include "alnumbers.h"
#include "alnumeric.h"
#include "opthelpers.h"


namespace {

using ushort = unsigned short;
using ushort2 = std::pair<ushort,ushort>;

/* Because std::array doesn't have constexpr non-const accessors in C++14. */
template<typename T, size_t N>
struct our_array {
    T mData[N];
};

constexpr size_t BitReverseCounter(size_t log2_size) noexcept
{
    /* Some magic math that calculates the number of swaps needed for a
     * sequence of bit-reversed indices when index < reversed_index.
     */
    return (1u<<(log2_size-1)) - (1u<<((log2_size-1u)/2u));
}

template<size_t N>
constexpr auto GetBitReverser() noexcept
{
    static_assert(N <= sizeof(ushort)*8, "Too many bits for the bit-reversal table.");

    our_array<ushort2, BitReverseCounter(N)> ret{};
    const size_t fftsize{1u << N};
    size_t ret_i{0};

    /* Bit-reversal permutation applied to a sequence of fftsize items. */
    for(size_t idx{1u};idx < fftsize-1;++idx)
    {
        size_t revidx{0u}, imask{idx};
        for(size_t i{0};i < N;++i)
        {
            revidx = (revidx<<1) | (imask&1);
            imask >>= 1;
        }

        if(idx < revidx)
        {
            ret.mData[ret_i].first  = static_cast<ushort>(idx);
            ret.mData[ret_i].second = static_cast<ushort>(revidx);
            ++ret_i;
        }
    }
    assert(ret_i == al::size(ret.mData));
    return ret;
}

/* These bit-reversal swap tables support up to 10-bit indices (1024 elements),
 * which is the largest used by OpenAL Soft's filters and effects. Larger FFT
 * requests, used by some utilities where performance is less important, will
 * use a slower table-less path.
 */
constexpr auto BitReverser2 = GetBitReverser<2>();
constexpr auto BitReverser3 = GetBitReverser<3>();
constexpr auto BitReverser4 = GetBitReverser<4>();
constexpr auto BitReverser5 = GetBitReverser<5>();
constexpr auto BitReverser6 = GetBitReverser<6>();
constexpr auto BitReverser7 = GetBitReverser<7>();
constexpr auto BitReverser8 = GetBitReverser<8>();
constexpr auto BitReverser9 = GetBitReverser<9>();
constexpr auto BitReverser10 = GetBitReverser<10>();
constexpr al::span<const ushort2> gBitReverses[11]{
    {}, {},
    BitReverser2.mData,
    BitReverser3.mData,
    BitReverser4.mData,
    BitReverser5.mData,
    BitReverser6.mData,
    BitReverser7.mData,
    BitReverser8.mData,
    BitReverser9.mData,
    BitReverser10.mData
};

} // namespace

void complex_fft(const al::span<std::complex<double>> buffer, const double sign)
{
    const size_t fftsize{buffer.size()};
    /* Get the number of bits used for indexing. Simplifies bit-reversal and
     * the main loop count.
     */
    const size_t log2_size{static_cast<size_t>(al::countr_zero(fftsize))};

    if(unlikely(log2_size >= al::size(gBitReverses)))
    {
        for(size_t idx{1u};idx < fftsize-1;++idx)
        {
            size_t revidx{0u}, imask{idx};
            for(size_t i{0};i < log2_size;++i)
            {
                revidx = (revidx<<1) | (imask&1);
                imask >>= 1;
            }

            if(idx < revidx)
                std::swap(buffer[idx], buffer[revidx]);
        }
    }
    else for(auto &rev : gBitReverses[log2_size])
        std::swap(buffer[rev.first], buffer[rev.second]);

    /* Iterative form of Danielson-Lanczos lemma */
    const double pi{al::numbers::pi * sign};
    size_t step2{1u};
    for(size_t i{0};i < log2_size;++i)
    {
        const double arg{pi / static_cast<double>(step2)};

        /* TODO: Would std::polar(1.0, arg) be any better? */
        const std::complex<double> w{std::cos(arg), std::sin(arg)};
        std::complex<double> u{1.0, 0.0};
        const size_t step{step2 << 1};
        for(size_t j{0};j < step2;j++)
        {
            for(size_t k{j};k < fftsize;k+=step)
            {
                std::complex<double> temp{buffer[k+step2] * u};
                buffer[k+step2] = buffer[k] - temp;
                buffer[k] += temp;
            }

            u *= w;
        }

        step2 <<= 1;
    }
}

void complex_hilbert(const al::span<std::complex<double>> buffer)
{
    inverse_fft(buffer);

    const double inverse_size = 1.0/static_cast<double>(buffer.size());
    auto bufiter = buffer.begin();
    const auto halfiter = bufiter + (buffer.size()>>1);

    *bufiter *= inverse_size; ++bufiter;
    bufiter = std::transform(bufiter, halfiter, bufiter,
        [inverse_size](const std::complex<double> &c) -> std::complex<double>
        { return c * (2.0*inverse_size); });
    *bufiter *= inverse_size; ++bufiter;

    std::fill(bufiter, buffer.end(), std::complex<double>{});

    forward_fft(buffer);
}
