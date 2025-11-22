
#include "polyphase_resampler.h"

#include <algorithm>
#include <cmath>

#include "alnumbers.h"
#include "opthelpers.h"


namespace {

constexpr double Epsilon{1e-9};

using uint = unsigned int;

/* This is the normalized cardinal sine (sinc) function.
 *
 *   sinc(x) = { 1,                   x = 0
 *             { sin(pi x) / (pi x),  otherwise.
 */
double Sinc(const double x)
{
    if(unlikely(std::abs(x) < Epsilon))
        return 1.0;
    return std::sin(al::numbers::pi*x) / (al::numbers::pi*x);
}

/* The zero-order modified Bessel function of the first kind, used for the
 * Kaiser window.
 *
 *   I_0(x) = sum_{k=0}^inf (1 / k!)^2 (x / 2)^(2 k)
 *          = sum_{k=0}^inf ((x / 2)^k / k!)^2
 */
constexpr double BesselI_0(const double x)
{
    // Start at k=1 since k=0 is trivial.
    const double x2{x/2.0};
    double term{1.0};
    double sum{1.0};
    int k{1};

    // Let the integration converge until the term of the sum is no longer
    // significant.
    double last_sum{};
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
double Kaiser(const double b, const double k)
{
    if(!(k >= -1.0 && k <= 1.0))
        return 0.0;
    return BesselI_0(b * std::sqrt(1.0 - k*k)) / BesselI_0(b);
}

// Calculates the greatest common divisor of a and b.
constexpr uint Gcd(uint x, uint y)
{
    while(y > 0)
    {
        const uint z{y};
        y = x % y;
        x = z;
    }
    return x;
}

/* Calculates the size (order) of the Kaiser window.  Rejection is in dB and
 * the transition width is normalized frequency (0.5 is nyquist).
 *
 *   M = { ceil((r - 7.95) / (2.285 2 pi f_t)),  r > 21
 *       { ceil(5.79 / 2 pi f_t),                r <= 21.
 *
 */
constexpr uint CalcKaiserOrder(const double rejection, const double transition)
{
    const double w_t{2.0 * al::numbers::pi * transition};
    if LIKELY(rejection > 21.0)
        return static_cast<uint>(std::ceil((rejection - 7.95) / (2.285 * w_t)));
    return static_cast<uint>(std::ceil(5.79 / w_t));
}

// Calculates the beta value of the Kaiser window.  Rejection is in dB.
constexpr double CalcKaiserBeta(const double rejection)
{
    if LIKELY(rejection > 50.0)
        return 0.1102 * (rejection - 8.7);
    if(rejection >= 21.0)
        return (0.5842 * std::pow(rejection - 21.0, 0.4)) +
               (0.07886 * (rejection - 21.0));
    return 0.0;
}

/* Calculates a point on the Kaiser-windowed sinc filter for the given half-
 * width, beta, gain, and cutoff.  The point is specified in non-normalized
 * samples, from 0 to M, where M = (2 l + 1).
 *
 *   w(k) 2 p f_t sinc(2 f_t x)
 *
 *   x    -- centered sample index (i - l)
 *   k    -- normalized and centered window index (x / l)
 *   w(k) -- window function (Kaiser)
 *   p    -- gain compensation factor when sampling
 *   f_t  -- normalized center frequency (or cutoff; 0.5 is nyquist)
 */
double SincFilter(const uint l, const double b, const double gain, const double cutoff,
    const uint i)
{
    const double x{static_cast<double>(i) - l};
    return Kaiser(b, x / l) * 2.0 * gain * cutoff * Sinc(2.0 * cutoff * x);
}

} // namespace

// Calculate the resampling metrics and build the Kaiser-windowed sinc filter
// that's used to cut frequencies above the destination nyquist.
void PPhaseResampler::init(const uint srcRate, const uint dstRate)
{
    const uint gcd{Gcd(srcRate, dstRate)};
    mP = dstRate / gcd;
    mQ = srcRate / gcd;

    /* The cutoff is adjusted by half the transition width, so the transition
     * ends before the nyquist (0.5).  Both are scaled by the downsampling
     * factor.
     */
    double cutoff, width;
    if(mP > mQ)
    {
        cutoff = 0.475 / mP;
        width = 0.05 / mP;
    }
    else
    {
        cutoff = 0.475 / mQ;
        width = 0.05 / mQ;
    }
    // A rejection of -180 dB is used for the stop band. Round up when
    // calculating the left offset to avoid increasing the transition width.
    const uint l{(CalcKaiserOrder(180.0, width)+1) / 2};
    const double beta{CalcKaiserBeta(180.0)};
    mM = l*2 + 1;
    mL = l;
    mF.resize(mM);
    for(uint i{0};i < mM;i++)
        mF[i] = SincFilter(l, beta, mP, cutoff, i);
}

// Perform the upsample-filter-downsample resampling operation using a
// polyphase filter implementation.
void PPhaseResampler::process(const uint inN, const double *in, const uint outN, double *out)
{
    if UNLIKELY(outN == 0)
        return;

    // Handle in-place operation.
    std::vector<double> workspace;
    double *work{out};
    if UNLIKELY(work == in)
    {
        workspace.resize(outN);
        work = workspace.data();
    }

    // Resample the input.
    const uint p{mP}, q{mQ}, m{mM}, l{mL};
    const double *f{mF.data()};
    for(uint i{0};i < outN;i++)
    {
        // Input starts at l to compensate for the filter delay.  This will
        // drop any build-up from the first half of the filter.
        size_t j_f{(l + q*i) % p};
        size_t j_s{(l + q*i) / p};

        // Only take input when 0 <= j_s < inN.
        double r{0.0};
        if LIKELY(j_f < m)
        {
            size_t filt_len{(m-j_f+p-1) / p};
            if LIKELY(j_s+1 > inN)
            {
                size_t skip{std::min<size_t>(j_s+1 - inN, filt_len)};
                j_f += p*skip;
                j_s -= skip;
                filt_len -= skip;
            }
            if(size_t todo{std::min<size_t>(j_s+1, filt_len)})
            {
                do {
                    r += f[j_f] * in[j_s];
                    j_f += p;
                    --j_s;
                } while(--todo);
            }
        }
        work[i] = r;
    }
    // Clean up after in-place operation.
    if(work != out)
        std::copy_n(work, outN, out);
}
