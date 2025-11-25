
#include "bsinc_tables.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

#include "alnumbers.h"
#include "core/mixer/defs.h"


namespace {

using uint = unsigned int;


/* This is the normalized cardinal sine (sinc) function.
 *
 *   sinc(x) = { 1,                   x = 0
 *             { sin(pi x) / (pi x),  otherwise.
 */
constexpr double Sinc(const double x)
{
    constexpr double epsilon{std::numeric_limits<double>::epsilon()};
    if(!(x > epsilon || x < -epsilon))
        return 1.0;
    return std::sin(al::numbers::pi*x) / (al::numbers::pi*x);
}

/* The zero-order modified Bessel function of the first kind, used for the
 * Kaiser window.
 *
 *   I_0(x) = sum_{k=0}^inf (1 / k!)^2 (x / 2)^(2 k)
 *          = sum_{k=0}^inf ((x / 2)^k / k!)^2
 */
constexpr double BesselI_0(const double x) noexcept
{
    /* Start at k=1 since k=0 is trivial. */
    const double x2{x / 2.0};
    double term{1.0};
    double sum{1.0};
    double last_sum{};
    int k{1};

    /* Let the integration converge until the term of the sum is no longer
     * significant.
     */
    do {
        const double y{x2 / k};
        ++k;
        last_sum = sum;
        term *= y * y;
        sum += term;
    } while(sum != last_sum);

    return sum;
}

/* Calculate a Kaiser window from the given beta value and a normalized k
 * [-1, 1].
 *
 *   w(k) = { I_0(B sqrt(1 - k^2)) / I_0(B),  -1 <= k <= 1
 *          { 0,                              elsewhere.
 *
 * Where k can be calculated as:
 *
 *   k = i / l,         where -l <= i <= l.
 *
 * or:
 *
 *   k = 2 i / M - 1,   where 0 <= i <= M.
 */
constexpr double Kaiser(const double beta, const double k, const double besseli_0_beta)
{
    if(!(k >= -1.0 && k <= 1.0))
        return 0.0;
    return BesselI_0(beta * std::sqrt(1.0 - k*k)) / besseli_0_beta;
}

/* Calculates the (normalized frequency) transition width of the Kaiser window.
 * Rejection is in dB.
 */
constexpr double CalcKaiserWidth(const double rejection, const uint order) noexcept
{
    if(rejection > 21.19)
        return (rejection - 7.95) / (2.285 * al::numbers::pi*2.0 * order);
    /* This enforces a minimum rejection of just above 21.18dB */
    return 5.79 / (al::numbers::pi*2.0 * order);
}

/* Calculates the beta value of the Kaiser window. Rejection is in dB. */
constexpr double CalcKaiserBeta(const double rejection)
{
    if(rejection > 50.0)
        return 0.1102 * (rejection-8.7);
    else if(rejection >= 21.0)
        return (0.5842 * std::pow(rejection-21.0, 0.4)) + (0.07886 * (rejection-21.0));
    return 0.0;
}


struct BSincHeader {
    double width{};
    double beta{};
    double scaleBase{};
    double scaleRange{};
    double besseli_0_beta{};

    uint a[BSincScaleCount]{};
    uint total_size{};

    constexpr BSincHeader(uint Rejection, uint Order) noexcept
    {
        width = CalcKaiserWidth(Rejection, Order);
        beta = CalcKaiserBeta(Rejection);
        scaleBase = width / 2.0;
        scaleRange = 1.0 - scaleBase;
        besseli_0_beta = BesselI_0(beta);

        uint num_points{Order+1};
        for(uint si{0};si < BSincScaleCount;++si)
        {
            const double scale{scaleBase + (scaleRange * (si+1) / BSincScaleCount)};
            const uint a_{std::min(static_cast<uint>(num_points / 2.0 / scale), num_points)};
            const uint m{2 * a_};

            a[si] = a_;
            total_size += 4 * BSincPhaseCount * ((m+3) & ~3u);
        }
    }
};

/* 11th and 23rd order filters (12 and 24-point respectively) with a 60dB drop
 * at nyquist. Each filter will scale up the order when downsampling, to 23rd
 * and 47th order respectively.
 */
constexpr BSincHeader bsinc12_hdr{60, 11};
constexpr BSincHeader bsinc24_hdr{60, 23};


/* NOTE: GCC 5 has an issue with BSincHeader objects being in an anonymous
 * namespace while also being used as non-type template parameters.
 */
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 6

/* The number of sample points is double the a value (rounded up to a multiple
 * of 4), and scale index 0 includes the doubling for downsampling. bsinc24 is
 * currently the highest quality filter, and will use the most sample points.
 */
constexpr uint BSincPointsMax{(bsinc24_hdr.a[0]*2 + 3) & ~3u};
static_assert(BSincPointsMax <= MaxResamplerPadding, "MaxResamplerPadding is too small");

template<size_t total_size>
struct BSincFilterArray {
    alignas(16) std::array<float, total_size> mTable;
    const BSincHeader &hdr;

    BSincFilterArray(const BSincHeader &hdr_) : hdr{hdr_}
    {
#else
template<const BSincHeader &hdr>
struct BSincFilterArray {
    alignas(16) std::array<float, hdr.total_size> mTable{};

    BSincFilterArray()
    {
        constexpr uint BSincPointsMax{(hdr.a[0]*2 + 3) & ~3u};
        static_assert(BSincPointsMax <= MaxResamplerPadding, "MaxResamplerPadding is too small");
#endif
        using filter_type = double[BSincPhaseCount+1][BSincPointsMax];
        auto filter = std::make_unique<filter_type[]>(BSincScaleCount);

        /* Calculate the Kaiser-windowed Sinc filter coefficients for each
         * scale and phase index.
         */
        for(uint si{0};si < BSincScaleCount;++si)
        {
            const uint m{hdr.a[si] * 2};
            const size_t o{(BSincPointsMax-m) / 2};
            const double scale{hdr.scaleBase + (hdr.scaleRange * (si+1) / BSincScaleCount)};
            const double cutoff{scale - (hdr.scaleBase * std::max(1.0, scale*2.0))};
            const auto a = static_cast<double>(hdr.a[si]);
            const double l{a - 1.0/BSincPhaseCount};

            /* Do one extra phase index so that the phase delta has a proper
             * target for its last index.
             */
            for(uint pi{0};pi <= BSincPhaseCount;++pi)
            {
                const double phase{std::floor(l) + (pi/double{BSincPhaseCount})};

                for(uint i{0};i < m;++i)
                {
                    const double x{i - phase};
                    filter[si][pi][o+i] = Kaiser(hdr.beta, x/l, hdr.besseli_0_beta) * cutoff *
                        Sinc(cutoff*x);
                }
            }
        }

        size_t idx{0};
        for(size_t si{0};si < BSincScaleCount;++si)
        {
            const size_t m{((hdr.a[si]*2) + 3) & ~3u};
            const size_t o{(BSincPointsMax-m) / 2};

            /* Write out each phase index's filter and phase delta for this
             * quality scale.
             */
            for(size_t pi{0};pi < BSincPhaseCount;++pi)
            {
                for(size_t i{0};i < m;++i)
                    mTable[idx++] = static_cast<float>(filter[si][pi][o+i]);

                /* Linear interpolation between phases is simplified by pre-
                 * calculating the delta (b - a) in: x = a + f (b - a)
                 */
                for(size_t i{0};i < m;++i)
                {
                    const double phDelta{filter[si][pi+1][o+i] - filter[si][pi][o+i]};
                    mTable[idx++] = static_cast<float>(phDelta);
                }
            }
            /* Calculate and write out each phase index's filter quality scale
             * deltas. The last scale index doesn't have any scale or scale-
             * phase deltas.
             */
            if(si == BSincScaleCount-1)
            {
                for(size_t i{0};i < BSincPhaseCount*m*2;++i)
                    mTable[idx++] = 0.0f;
            }
            else for(size_t pi{0};pi < BSincPhaseCount;++pi)
            {
                /* Linear interpolation between scales is also simplified.
                 *
                 * Given a difference in the number of points between scales,
                 * the destination points will be 0, thus: x = a + f (-a)
                 */
                for(size_t i{0};i < m;++i)
                {
                    const double scDelta{filter[si+1][pi][o+i] - filter[si][pi][o+i]};
                    mTable[idx++] = static_cast<float>(scDelta);
                }

                /* This last simplification is done to complete the bilinear
                 * equation for the combination of phase and scale.
                 */
                for(size_t i{0};i < m;++i)
                {
                    const double spDelta{(filter[si+1][pi+1][o+i] - filter[si+1][pi][o+i]) -
                        (filter[si][pi+1][o+i] - filter[si][pi][o+i])};
                    mTable[idx++] = static_cast<float>(spDelta);
                }
            }
        }
        assert(idx == hdr.total_size);
    }

    constexpr const BSincHeader &getHeader() const noexcept { return hdr; }
    constexpr const float *getTable() const noexcept { return &mTable.front(); }
};

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 6
const BSincFilterArray<bsinc12_hdr.total_size> bsinc12_filter{bsinc12_hdr};
const BSincFilterArray<bsinc24_hdr.total_size> bsinc24_filter{bsinc24_hdr};
#else
const BSincFilterArray<bsinc12_hdr> bsinc12_filter{};
const BSincFilterArray<bsinc24_hdr> bsinc24_filter{};
#endif

template<typename T>
constexpr BSincTable GenerateBSincTable(const T &filter)
{
    BSincTable ret{};
    const BSincHeader &hdr = filter.getHeader();
    ret.scaleBase = static_cast<float>(hdr.scaleBase);
    ret.scaleRange = static_cast<float>(1.0 / hdr.scaleRange);
    for(size_t i{0};i < BSincScaleCount;++i)
        ret.m[i] = ((hdr.a[i]*2) + 3) & ~3u;
    ret.filterOffset[0] = 0;
    for(size_t i{1};i < BSincScaleCount;++i)
        ret.filterOffset[i] = ret.filterOffset[i-1] + ret.m[i-1]*4*BSincPhaseCount;
    ret.Tab = filter.getTable();
    return ret;
}

} // namespace

const BSincTable bsinc12{GenerateBSincTable(bsinc12_filter)};
const BSincTable bsinc24{GenerateBSincTable(bsinc24_filter)};
