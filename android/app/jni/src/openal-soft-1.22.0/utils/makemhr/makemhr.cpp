/*
 * HRTF utility for producing and demonstrating the process of creating an
 * OpenAL Soft compatible HRIR data set.
 *
 * Copyright (C) 2011-2019  Christopher Fitzgerald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Or visit:  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * --------------------------------------------------------------------------
 *
 * A big thanks goes out to all those whose work done in the field of
 * binaural sound synthesis using measured HRTFs makes this utility and the
 * OpenAL Soft implementation possible.
 *
 * The algorithm for diffuse-field equalization was adapted from the work
 * done by Rio Emmanuel and Larcher Veronique of IRCAM and Bill Gardner of
 * MIT Media Laboratory.  It operates as follows:
 *
 *  1.  Take the FFT of each HRIR and only keep the magnitude responses.
 *  2.  Calculate the diffuse-field power-average of all HRIRs weighted by
 *      their contribution to the total surface area covered by their
 *      measurement. This has since been modified to use coverage volume for
 *      multi-field HRIR data sets.
 *  3.  Take the diffuse-field average and limit its magnitude range.
 *  4.  Equalize the responses by using the inverse of the diffuse-field
 *      average.
 *  5.  Reconstruct the minimum-phase responses.
 *  5.  Zero the DC component.
 *  6.  IFFT the result and truncate to the desired-length minimum-phase FIR.
 *
 * The spherical head algorithm for calculating propagation delay was adapted
 * from the paper:
 *
 *  Modeling Interaural Time Difference Assuming a Spherical Head
 *  Joel David Miller
 *  Music 150, Musical Acoustics, Stanford University
 *  December 2, 2001
 *
 * The formulae for calculating the Kaiser window metrics are from the
 * the textbook:
 *
 *  Discrete-Time Signal Processing
 *  Alan V. Oppenheim and Ronald W. Schafer
 *  Prentice-Hall Signal Processing Series
 *  1999
 */

#define _UNICODE
#include "config.h"

#include "makemhr.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

#ifdef HAVE_GETOPT
#include <unistd.h>
#else
#include "../getopt.h"
#endif

#include "alfstream.h"
#include "alspan.h"
#include "alstring.h"
#include "loaddef.h"
#include "loadsofa.h"

#include "win_main_utf8.h"


namespace {

using namespace std::placeholders;

} // namespace

#ifndef M_PI
#define M_PI                         (3.14159265358979323846)
#endif


// Head model used for calculating the impulse delays.
enum HeadModelT {
    HM_NONE,
    HM_DATASET, // Measure the onset from the dataset.
    HM_SPHERE   // Calculate the onset using a spherical head model.
};


// The epsilon used to maintain signal stability.
#define EPSILON                      (1e-9)

// The limits to the FFT window size override on the command line.
#define MIN_FFTSIZE                  (65536)
#define MAX_FFTSIZE                  (131072)

// The limits to the equalization range limit on the command line.
#define MIN_LIMIT                    (2.0)
#define MAX_LIMIT                    (120.0)

// The limits to the truncation window size on the command line.
#define MIN_TRUNCSIZE                (16)
#define MAX_TRUNCSIZE                (128)

// The limits to the custom head radius on the command line.
#define MIN_CUSTOM_RADIUS            (0.05)
#define MAX_CUSTOM_RADIUS            (0.15)

// The defaults for the command line options.
#define DEFAULT_FFTSIZE              (65536)
#define DEFAULT_EQUALIZE             (1)
#define DEFAULT_SURFACE              (1)
#define DEFAULT_LIMIT                (24.0)
#define DEFAULT_TRUNCSIZE            (32)
#define DEFAULT_HEAD_MODEL           (HM_DATASET)
#define DEFAULT_CUSTOM_RADIUS        (0.0)

// The maximum propagation delay value supported by OpenAL Soft.
#define MAX_HRTD                     (63.0)

// The OpenAL Soft HRTF format marker.  It stands for minimum-phase head
// response protocol 03.
#define MHR_FORMAT                   ("MinPHR03")

/* Channel index enums. Mono uses LeftChannel only. */
enum ChannelIndex : uint {
    LeftChannel = 0u,
    RightChannel = 1u
};


/* Performs a string substitution.  Any case-insensitive occurrences of the
 * pattern string are replaced with the replacement string.  The result is
 * truncated if necessary.
 */
static std::string StrSubst(al::span<const char> in, const al::span<const char> pat,
    const al::span<const char> rep)
{
    std::string ret;
    ret.reserve(in.size() + pat.size());

    while(in.size() >= pat.size())
    {
        if(al::strncasecmp(in.data(), pat.data(), pat.size()) == 0)
        {
            in = in.subspan(pat.size());
            ret.append(rep.data(), rep.size());
        }
        else
        {
            size_t endpos{1};
            while(endpos < in.size() && in[endpos] != pat.front())
                ++endpos;
            ret.append(in.data(), endpos);
            in = in.subspan(endpos);
        }
    }
    ret.append(in.data(), in.size());

    return ret;
}


/*********************
 *** Math routines ***
 *********************/

// Simple clamp routine.
static double Clamp(const double val, const double lower, const double upper)
{
    return std::min(std::max(val, lower), upper);
}

static inline uint dither_rng(uint *seed)
{
    *seed = *seed * 96314165 + 907633515;
    return *seed;
}

// Performs a triangular probability density function dither. The input samples
// should be normalized (-1 to +1).
static void TpdfDither(double *RESTRICT out, const double *RESTRICT in, const double scale,
                       const uint count, const uint step, uint *seed)
{
    static constexpr double PRNG_SCALE = 1.0 / std::numeric_limits<uint>::max();

    for(uint i{0};i < count;i++)
    {
        uint prn0{dither_rng(seed)};
        uint prn1{dither_rng(seed)};
        *out = std::round(*(in++)*scale + (prn0*PRNG_SCALE - prn1*PRNG_SCALE));
        out += step;
    }
}

/* Fast Fourier transform routines. The number of points must be a power of
 * two.
 */

// Performs bit-reversal ordering.
static void FftArrange(const uint n, complex_d *inout)
{
    // Handle in-place arrangement.
    uint rk{0u};
    for(uint k{0u};k < n;k++)
    {
        if(rk > k)
            std::swap(inout[rk], inout[k]);

        uint m{n};
        while(rk&(m >>= 1))
            rk &= ~m;
        rk |= m;
    }
}

// Performs the summation.
static void FftSummation(const uint n, const double s, complex_d *cplx)
{
    double pi;
    uint m, m2;
    uint i, k, mk;

    pi = s * M_PI;
    for(m = 1, m2 = 2;m < n; m <<= 1, m2 <<= 1)
    {
        // v = Complex (-2.0 * sin (0.5 * pi / m) * sin (0.5 * pi / m), -sin (pi / m))
        double sm = std::sin(0.5 * pi / m);
        auto v = complex_d{-2.0*sm*sm, -std::sin(pi / m)};
        auto w = complex_d{1.0, 0.0};
        for(i = 0;i < m;i++)
        {
            for(k = i;k < n;k += m2)
            {
                mk = k + m;
                auto t = w * cplx[mk];
                cplx[mk] = cplx[k] - t;
                cplx[k] = cplx[k] + t;
            }
            w += v*w;
        }
    }
}

// Performs a forward FFT.
void FftForward(const uint n, complex_d *inout)
{
    FftArrange(n, inout);
    FftSummation(n, 1.0, inout);
}

// Performs an inverse FFT.
void FftInverse(const uint n, complex_d *inout)
{
    FftArrange(n, inout);
    FftSummation(n, -1.0, inout);
    double f{1.0 / n};
    for(uint i{0};i < n;i++)
        inout[i] *= f;
}

/* Calculate the complex helical sequence (or discrete-time analytical signal)
 * of the given input using the Hilbert transform. Given the natural logarithm
 * of a signal's magnitude response, the imaginary components can be used as
 * the angles for minimum-phase reconstruction.
 */
static void Hilbert(const uint n, complex_d *inout)
{
    uint i;

    // Handle in-place operation.
    for(i = 0;i < n;i++)
        inout[i].imag(0.0);

    FftInverse(n, inout);
    for(i = 1;i < (n+1)/2;i++)
        inout[i] *= 2.0;
    /* Increment i if n is even. */
    i += (n&1)^1;
    for(;i < n;i++)
        inout[i] = complex_d{0.0, 0.0};
    FftForward(n, inout);
}

/* Calculate the magnitude response of the given input.  This is used in
 * place of phase decomposition, since the phase residuals are discarded for
 * minimum phase reconstruction.  The mirrored half of the response is also
 * discarded.
 */
void MagnitudeResponse(const uint n, const complex_d *in, double *out)
{
    const uint m = 1 + (n / 2);
    uint i;
    for(i = 0;i < m;i++)
        out[i] = std::max(std::abs(in[i]), EPSILON);
}

/* Apply a range limit (in dB) to the given magnitude response.  This is used
 * to adjust the effects of the diffuse-field average on the equalization
 * process.
 */
static void LimitMagnitudeResponse(const uint n, const uint m, const double limit, const double *in, double *out)
{
    double halfLim;
    uint i, lower, upper;
    double ave;

    halfLim = limit / 2.0;
    // Convert the response to dB.
    for(i = 0;i < m;i++)
        out[i] = 20.0 * std::log10(in[i]);
    // Use six octaves to calculate the average magnitude of the signal.
    lower = (static_cast<uint>(std::ceil(n / std::pow(2.0, 8.0)))) - 1;
    upper = (static_cast<uint>(std::floor(n / std::pow(2.0, 2.0)))) - 1;
    ave = 0.0;
    for(i = lower;i <= upper;i++)
        ave += out[i];
    ave /= upper - lower + 1;
    // Keep the response within range of the average magnitude.
    for(i = 0;i < m;i++)
        out[i] = Clamp(out[i], ave - halfLim, ave + halfLim);
    // Convert the response back to linear magnitude.
    for(i = 0;i < m;i++)
        out[i] = std::pow(10.0, out[i] / 20.0);
}

/* Reconstructs the minimum-phase component for the given magnitude response
 * of a signal.  This is equivalent to phase recomposition, sans the missing
 * residuals (which were discarded).  The mirrored half of the response is
 * reconstructed.
 */
static void MinimumPhase(const uint n, double *mags, complex_d *out)
{
    const uint m{(n/2) + 1};

    uint i;
    for(i = 0;i < m;i++)
        out[i] = std::log(mags[i]);
    for(;i < n;i++)
    {
        mags[i] = mags[n - i];
        out[i] = out[n - i];
    }
    Hilbert(n, out);
    // Remove any DC offset the filter has.
    mags[0] = EPSILON;
    for(i = 0;i < n;i++)
    {
        auto a = std::exp(complex_d{0.0, out[i].imag()});
        out[i] = a * mags[i];
    }
}


/***************************
 *** File storage output ***
 ***************************/

// Write an ASCII string to a file.
static int WriteAscii(const char *out, FILE *fp, const char *filename)
{
    size_t len;

    len = strlen(out);
    if(fwrite(out, 1, len, fp) != len)
    {
        fclose(fp);
        fprintf(stderr, "\nError: Bad write to file '%s'.\n", filename);
        return 0;
    }
    return 1;
}

// Write a binary value of the given byte order and byte size to a file,
// loading it from a 32-bit unsigned integer.
static int WriteBin4(const uint bytes, const uint32_t in, FILE *fp, const char *filename)
{
    uint8_t out[4];
    uint i;

    for(i = 0;i < bytes;i++)
        out[i] = (in>>(i*8)) & 0x000000FF;

    if(fwrite(out, 1, bytes, fp) != bytes)
    {
        fprintf(stderr, "\nError: Bad write to file '%s'.\n", filename);
        return 0;
    }
    return 1;
}

// Store the OpenAL Soft HRTF data set.
static int StoreMhr(const HrirDataT *hData, const char *filename)
{
    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};
    const uint n{hData->mIrPoints};
    uint dither_seed{22222};
    uint fi, ei, ai, i;
    FILE *fp;

    if((fp=fopen(filename, "wb")) == nullptr)
    {
        fprintf(stderr, "\nError: Could not open MHR file '%s'.\n", filename);
        return 0;
    }
    if(!WriteAscii(MHR_FORMAT, fp, filename))
        return 0;
    if(!WriteBin4(4, hData->mIrRate, fp, filename))
        return 0;
    if(!WriteBin4(1, static_cast<uint32_t>(hData->mChannelType), fp, filename))
        return 0;
    if(!WriteBin4(1, hData->mIrPoints, fp, filename))
        return 0;
    if(!WriteBin4(1, hData->mFdCount, fp, filename))
        return 0;
    for(fi = hData->mFdCount-1;fi < hData->mFdCount;fi--)
    {
        auto fdist = static_cast<uint32_t>(std::round(1000.0 * hData->mFds[fi].mDistance));
        if(!WriteBin4(2, fdist, fp, filename))
            return 0;
        if(!WriteBin4(1, hData->mFds[fi].mEvCount, fp, filename))
            return 0;
        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            if(!WriteBin4(1, hData->mFds[fi].mEvs[ei].mAzCount, fp, filename))
                return 0;
        }
    }

    for(fi = hData->mFdCount-1;fi < hData->mFdCount;fi--)
    {
        constexpr double scale{8388607.0};
        constexpr uint bps{3u};

        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                double out[2 * MAX_TRUNCSIZE];

                TpdfDither(out, azd->mIrs[0], scale, n, channels, &dither_seed);
                if(hData->mChannelType == CT_STEREO)
                    TpdfDither(out+1, azd->mIrs[1], scale, n, channels, &dither_seed);
                for(i = 0;i < (channels * n);i++)
                {
                    const auto v = static_cast<int>(Clamp(out[i], -scale-1.0, scale));
                    if(!WriteBin4(bps, static_cast<uint32_t>(v), fp, filename))
                        return 0;
                }
            }
        }
    }
    for(fi = hData->mFdCount-1;fi < hData->mFdCount;fi--)
    {
        /* Delay storage has 2 bits of extra precision. */
        constexpr double DelayPrecScale{4.0};
        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                const HrirAzT &azd = hData->mFds[fi].mEvs[ei].mAzs[ai];

                auto v = static_cast<uint>(std::round(azd.mDelays[0]*DelayPrecScale));
                if(!WriteBin4(1, v, fp, filename)) return 0;
                if(hData->mChannelType == CT_STEREO)
                {
                    v = static_cast<uint>(std::round(azd.mDelays[1]*DelayPrecScale));
                    if(!WriteBin4(1, v, fp, filename)) return 0;
                }
            }
        }
    }
    fclose(fp);
    return 1;
}


/***********************
 *** HRTF processing ***
 ***********************/

/* Balances the maximum HRIR magnitudes of multi-field data sets by
 * independently normalizing each field in relation to the overall maximum.
 * This is done to ignore distance attenuation.
 */
static void BalanceFieldMagnitudes(const HrirDataT *hData, const uint channels, const uint m)
{
    double maxMags[MAX_FD_COUNT];
    uint fi, ei, ai, ti, i;

    double maxMag{0.0};
    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        maxMags[fi] = 0.0;

        for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                for(ti = 0;ti < channels;ti++)
                {
                    for(i = 0;i < m;i++)
                        maxMags[fi] = std::max(azd->mIrs[ti][i], maxMags[fi]);
                }
            }
        }

        maxMag = std::max(maxMags[fi], maxMag);
    }

    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        const double magFactor{maxMag / maxMags[fi]};

        for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                for(ti = 0;ti < channels;ti++)
                {
                    for(i = 0;i < m;i++)
                        azd->mIrs[ti][i] *= magFactor;
                }
            }
        }
    }
}

/* Calculate the contribution of each HRIR to the diffuse-field average based
 * on its coverage volume.  All volumes are centered at the spherical HRIR
 * coordinates and measured by extruded solid angle.
 */
static void CalculateDfWeights(const HrirDataT *hData, double *weights)
{
    double sum, innerRa, outerRa, evs, ev, upperEv, lowerEv;
    double solidAngle, solidVolume;
    uint fi, ei;

    sum = 0.0;
    // The head radius acts as the limit for the inner radius.
    innerRa = hData->mRadius;
    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        // Each volume ends half way between progressive field measurements.
        if((fi + 1) < hData->mFdCount)
            outerRa = 0.5f * (hData->mFds[fi].mDistance + hData->mFds[fi + 1].mDistance);
        // The final volume has its limit extended to some practical value.
        // This is done to emphasize the far-field responses in the average.
        else
            outerRa = 10.0f;

        evs = M_PI / 2.0 / (hData->mFds[fi].mEvCount - 1);
        for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
        {
            // For each elevation, calculate the upper and lower limits of
            // the patch band.
            ev = hData->mFds[fi].mEvs[ei].mElevation;
            lowerEv = std::max(-M_PI / 2.0, ev - evs);
            upperEv = std::min(M_PI / 2.0, ev + evs);
            // Calculate the surface area of the patch band.
            solidAngle = 2.0 * M_PI * (std::sin(upperEv) - std::sin(lowerEv));
            // Then the volume of the extruded patch band.
            solidVolume = solidAngle * (std::pow(outerRa, 3.0) - std::pow(innerRa, 3.0)) / 3.0;
            // Each weight is the volume of one extruded patch.
            weights[(fi * MAX_EV_COUNT) + ei] = solidVolume / hData->mFds[fi].mEvs[ei].mAzCount;
            // Sum the total coverage volume of the HRIRs for all fields.
            sum += solidAngle;
        }

        innerRa = outerRa;
    }

    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        // Normalize the weights given the total surface coverage for all
        // fields.
        for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
            weights[(fi * MAX_EV_COUNT) + ei] /= sum;
    }
}

/* Calculate the diffuse-field average from the given magnitude responses of
 * the HRIR set.  Weighting can be applied to compensate for the varying
 * coverage of each HRIR.  The final average can then be limited by the
 * specified magnitude range (in positive dB; 0.0 to skip).
 */
static void CalculateDiffuseFieldAverage(const HrirDataT *hData, const uint channels, const uint m, const int weighted, const double limit, double *dfa)
{
    std::vector<double> weights(hData->mFdCount * MAX_EV_COUNT);
    uint count, ti, fi, ei, i, ai;

    if(weighted)
    {
        // Use coverage weighting to calculate the average.
        CalculateDfWeights(hData, weights.data());
    }
    else
    {
        double weight;

        // If coverage weighting is not used, the weights still need to be
        // averaged by the number of existing HRIRs.
        count = hData->mIrCount;
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = 0;ei < hData->mFds[fi].mEvStart;ei++)
                count -= hData->mFds[fi].mEvs[ei].mAzCount;
        }
        weight = 1.0 / count;

        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
                weights[(fi * MAX_EV_COUNT) + ei] = weight;
        }
    }
    for(ti = 0;ti < channels;ti++)
    {
        for(i = 0;i < m;i++)
            dfa[(ti * m) + i] = 0.0;
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
            {
                for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
                {
                    HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                    // Get the weight for this HRIR's contribution.
                    double weight = weights[(fi * MAX_EV_COUNT) + ei];

                    // Add this HRIR's weighted power average to the total.
                    for(i = 0;i < m;i++)
                        dfa[(ti * m) + i] += weight * azd->mIrs[ti][i] * azd->mIrs[ti][i];
                }
            }
        }
        // Finish the average calculation and keep it from being too small.
        for(i = 0;i < m;i++)
            dfa[(ti * m) + i] = std::max(sqrt(dfa[(ti * m) + i]), EPSILON);
        // Apply a limit to the magnitude range of the diffuse-field average
        // if desired.
        if(limit > 0.0)
            LimitMagnitudeResponse(hData->mFftSize, m, limit, &dfa[ti * m], &dfa[ti * m]);
    }
}

// Perform diffuse-field equalization on the magnitude responses of the HRIR
// set using the given average response.
static void DiffuseFieldEqualize(const uint channels, const uint m, const double *dfa, const HrirDataT *hData)
{
    uint ti, fi, ei, ai, i;

    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        for(ei = hData->mFds[fi].mEvStart;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

                for(ti = 0;ti < channels;ti++)
                {
                    for(i = 0;i < m;i++)
                        azd->mIrs[ti][i] /= dfa[(ti * m) + i];
                }
            }
        }
    }
}

// Resamples the HRIRs for use at the given sampling rate.
static void ResampleHrirs(const uint rate, HrirDataT *hData)
{
    struct Resampler {
        const double scale;
        const size_t m;

        /* Resampling from a lower rate to a higher rate. This likely only
         * works properly when 1 <= scale <= 2.
         */
        void upsample(double *resampled, const double *ir) const
        {
            std::fill_n(resampled, m, 0.0);
            resampled[0] = ir[0];
            for(size_t in{1};in < m;++in)
            {
                const auto offset = static_cast<double>(in) / scale;
                const auto out = static_cast<size_t>(offset);

                const double a{offset - static_cast<double>(out)};
                if(out == m-1)
                    resampled[out] += ir[in]*(1.0-a);
                else
                {
                    resampled[out  ] += ir[in]*(1.0-a);
                    resampled[out+1] += ir[in]*a;
                }
            }
        }

        /* Resampling from a higher rate to a lower rate. This likely only
         * works properly when 0.5 <= scale <= 1.0.
         */
        void downsample(double *resampled, const double *ir) const
        {
            resampled[0] = ir[0];
            for(size_t out{1};out < m;++out)
            {
                const auto offset = static_cast<double>(out) * scale;
                const auto in = static_cast<size_t>(offset);

                const double a{offset - static_cast<double>(in)};
                if(in == m-1)
                    resampled[out] = ir[in]*(1.0-a);
                else
                    resampled[out] = ir[in]*(1.0-a) + ir[in+1]*a;
            }
        }
    };

    while(rate > hData->mIrRate*2)
        ResampleHrirs(hData->mIrRate*2, hData);
    while(rate < (hData->mIrRate+1)/2)
        ResampleHrirs((hData->mIrRate+1)/2, hData);

    const auto scale = static_cast<double>(rate) / hData->mIrRate;
    const size_t m{hData->mFftSize/2u + 1u};
    auto resampled = std::vector<double>(m);

    const Resampler resampler{scale, m};
    auto do_resample = std::bind(
        std::mem_fn((scale > 1.0) ? &Resampler::upsample : &Resampler::downsample), &resampler,
        _1, _2);

    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};
    for(uint fi{0};fi < hData->mFdCount;++fi)
    {
        for(uint ei{hData->mFds[fi].mEvStart};ei < hData->mFds[fi].mEvCount;++ei)
        {
            for(uint ai{0};ai < hData->mFds[fi].mEvs[ei].mAzCount;++ai)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                for(uint ti{0};ti < channels;++ti)
                {
                    do_resample(resampled.data(), azd->mIrs[ti]);
                    /* This should probably be rescaled according to the scale,
                     * however it'll all be normalized in the end so a constant
                     * scalar is fine to leave.
                     */
                    std::transform(resampled.cbegin(), resampled.cend(), azd->mIrs[ti],
                        [](const double d) { return std::max(d, EPSILON); });
                }
            }
        }
    }
    hData->mIrRate = rate;
}

/* Given field and elevation indices and an azimuth, calculate the indices of
 * the two HRIRs that bound the coordinate along with a factor for
 * calculating the continuous HRIR using interpolation.
 */
static void CalcAzIndices(const HrirFdT &field, const uint ei, const double az, uint *a0, uint *a1, double *af)
{
    double f{(2.0*M_PI + az) * field.mEvs[ei].mAzCount / (2.0*M_PI)};
    uint i{static_cast<uint>(f) % field.mEvs[ei].mAzCount};

    f -= std::floor(f);
    *a0 = i;
    *a1 = (i + 1) % field.mEvs[ei].mAzCount;
    *af = f;
}

/* Synthesize any missing onset timings at the bottom elevations of each field.
 * This just mirrors some top elevations for the bottom, and blends the
 * remaining elevations (not an accurate model).
 */
static void SynthesizeOnsets(HrirDataT *hData)
{
    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};

    auto proc_field = [channels](HrirFdT &field) -> void
    {
        /* Get the starting elevation from the measurements, and use it as the
         * upper elevation limit for what needs to be calculated.
         */
        const uint upperElevReal{field.mEvStart};
        if(upperElevReal <= 0) return;

        /* Get the lowest half of the missing elevations' delays by mirroring
         * the top elevation delays. The responses are on a spherical grid
         * centered between the ears, so these should align.
         */
        uint ei{};
        if(channels > 1)
        {
            /* Take the polar opposite position of the desired measurement and
             * swap the ears.
             */
            field.mEvs[0].mAzs[0].mDelays[0] = field.mEvs[field.mEvCount-1].mAzs[0].mDelays[1];
            field.mEvs[0].mAzs[0].mDelays[1] = field.mEvs[field.mEvCount-1].mAzs[0].mDelays[0];
            for(ei = 1u;ei < (upperElevReal+1)/2;++ei)
            {
                const uint topElev{field.mEvCount-ei-1};

                for(uint ai{0u};ai < field.mEvs[ei].mAzCount;ai++)
                {
                    uint a0, a1;
                    double af;

                    /* Rotate this current azimuth by a half-circle, and lookup
                     * the mirrored elevation to find the indices for the polar
                     * opposite position (may need blending).
                     */
                    const double az{field.mEvs[ei].mAzs[ai].mAzimuth + M_PI};
                    CalcAzIndices(field, topElev, az, &a0, &a1, &af);

                    /* Blend the delays, and again, swap the ears. */
                    field.mEvs[ei].mAzs[ai].mDelays[0] = Lerp(
                        field.mEvs[topElev].mAzs[a0].mDelays[1],
                        field.mEvs[topElev].mAzs[a1].mDelays[1], af);
                    field.mEvs[ei].mAzs[ai].mDelays[1] = Lerp(
                        field.mEvs[topElev].mAzs[a0].mDelays[0],
                        field.mEvs[topElev].mAzs[a1].mDelays[0], af);
                }
            }
        }
        else
        {
            field.mEvs[0].mAzs[0].mDelays[0] = field.mEvs[field.mEvCount-1].mAzs[0].mDelays[0];
            for(ei = 1u;ei < (upperElevReal+1)/2;++ei)
            {
                const uint topElev{field.mEvCount-ei-1};

                for(uint ai{0u};ai < field.mEvs[ei].mAzCount;ai++)
                {
                    uint a0, a1;
                    double af;

                    /* For mono data sets, mirror the azimuth front<->back
                     * since the other ear is a mirror of what we have (e.g.
                     * the left ear's back-left is simulated with the right
                     * ear's front-right, which uses the left ear's front-left
                     * measurement).
                     */
                    double az{field.mEvs[ei].mAzs[ai].mAzimuth};
                    if(az <= M_PI) az = M_PI - az;
                    else az = (M_PI*2.0)-az + M_PI;
                    CalcAzIndices(field, topElev, az, &a0, &a1, &af);

                    field.mEvs[ei].mAzs[ai].mDelays[0] = Lerp(
                        field.mEvs[topElev].mAzs[a0].mDelays[0],
                        field.mEvs[topElev].mAzs[a1].mDelays[0], af);
                }
            }
        }
        /* Record the lowest elevation filled in with the mirrored top. */
        const uint lowerElevFake{ei-1u};

        /* Fill in the remaining delays using bilinear interpolation. This
         * helps smooth the transition back to the real delays.
         */
        for(;ei < upperElevReal;++ei)
        {
            const double ef{(field.mEvs[upperElevReal].mElevation - field.mEvs[ei].mElevation) /
                (field.mEvs[upperElevReal].mElevation - field.mEvs[lowerElevFake].mElevation)};

            for(uint ai{0u};ai < field.mEvs[ei].mAzCount;ai++)
            {
                uint a0, a1, a2, a3;
                double af0, af1;

                double az{field.mEvs[ei].mAzs[ai].mAzimuth};
                CalcAzIndices(field, upperElevReal, az, &a0, &a1, &af0);
                CalcAzIndices(field, lowerElevFake, az, &a2, &a3, &af1);
                double blend[4]{
                    (1.0-ef) * (1.0-af0),
                    (1.0-ef) * (    af0),
                    (    ef) * (1.0-af1),
                    (    ef) * (    af1)
                };

                for(uint ti{0u};ti < channels;ti++)
                {
                    field.mEvs[ei].mAzs[ai].mDelays[ti] =
                        field.mEvs[upperElevReal].mAzs[a0].mDelays[ti]*blend[0] +
                        field.mEvs[upperElevReal].mAzs[a1].mDelays[ti]*blend[1] +
                        field.mEvs[lowerElevFake].mAzs[a2].mDelays[ti]*blend[2] +
                        field.mEvs[lowerElevFake].mAzs[a3].mDelays[ti]*blend[3];
                }
            }
        }
    };
    std::for_each(hData->mFds.begin(), hData->mFds.begin()+hData->mFdCount, proc_field);
}

/* Attempt to synthesize any missing HRIRs at the bottom elevations of each
 * field.  Right now this just blends the lowest elevation HRIRs together and
 * applies a low-pass filter to simulate body occlusion.  It is a simple, if
 * inaccurate model.
 */
static void SynthesizeHrirs(HrirDataT *hData)
{
    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};
    auto htemp = std::vector<complex_d>(hData->mFftSize);
    const uint m{hData->mFftSize/2u + 1u};
    auto filter = std::vector<double>(m);
    const double beta{3.5e-6 * hData->mIrRate};

    auto proc_field = [channels,m,beta,&htemp,&filter](HrirFdT &field) -> void
    {
        const uint oi{field.mEvStart};
        if(oi <= 0) return;

        for(uint ti{0u};ti < channels;ti++)
        {
            uint a0, a1;
            double af;

            /* Use the lowest immediate-left response for the left ear and
             * lowest immediate-right response for the right ear. Given no comb
             * effects as a result of the left response reaching the right ear
             * and vice-versa, this produces a decent phantom-center response
             * underneath the head.
             */
            CalcAzIndices(field, oi, ((ti==0) ? -M_PI : M_PI) / 2.0, &a0, &a1, &af);
            for(uint i{0u};i < m;i++)
            {
                field.mEvs[0].mAzs[0].mIrs[ti][i] = Lerp(field.mEvs[oi].mAzs[a0].mIrs[ti][i],
                    field.mEvs[oi].mAzs[a1].mIrs[ti][i], af);
            }
        }

        for(uint ei{1u};ei < field.mEvStart;ei++)
        {
            const double of{static_cast<double>(ei) / field.mEvStart};
            const double b{(1.0 - of) * beta};
            double lp[4]{};

            /* Calculate a low-pass filter to simulate body occlusion. */
            lp[0] = Lerp(1.0, lp[0], b);
            lp[1] = Lerp(lp[0], lp[1], b);
            lp[2] = Lerp(lp[1], lp[2], b);
            lp[3] = Lerp(lp[2], lp[3], b);
            htemp[0] = lp[3];
            for(size_t i{1u};i < htemp.size();i++)
            {
                lp[0] = Lerp(0.0, lp[0], b);
                lp[1] = Lerp(lp[0], lp[1], b);
                lp[2] = Lerp(lp[1], lp[2], b);
                lp[3] = Lerp(lp[2], lp[3], b);
                htemp[i] = lp[3];
            }
            /* Get the filter's frequency-domain response and extract the
             * frequency magnitudes (phase will be reconstructed later)).
             */
            FftForward(static_cast<uint>(htemp.size()), htemp.data());
            std::transform(htemp.cbegin(), htemp.cbegin()+m, filter.begin(),
                [](const complex_d &c) -> double { return std::abs(c); });

            for(uint ai{0u};ai < field.mEvs[ei].mAzCount;ai++)
            {
                uint a0, a1;
                double af;

                CalcAzIndices(field, oi, field.mEvs[ei].mAzs[ai].mAzimuth, &a0, &a1, &af);
                for(uint ti{0u};ti < channels;ti++)
                {
                    for(uint i{0u};i < m;i++)
                    {
                        /* Blend the two defined HRIRs closest to this azimuth,
                         * then blend that with the synthesized -90 elevation.
                         */
                        const double s1{Lerp(field.mEvs[oi].mAzs[a0].mIrs[ti][i],
                            field.mEvs[oi].mAzs[a1].mIrs[ti][i], af)};
                        const double s{Lerp(field.mEvs[0].mAzs[0].mIrs[ti][i], s1, of)};
                        field.mEvs[ei].mAzs[ai].mIrs[ti][i] = s * filter[i];
                    }
                }
            }
        }
        const double b{beta};
        double lp[4]{};
        lp[0] = Lerp(1.0, lp[0], b);
        lp[1] = Lerp(lp[0], lp[1], b);
        lp[2] = Lerp(lp[1], lp[2], b);
        lp[3] = Lerp(lp[2], lp[3], b);
        htemp[0] = lp[3];
        for(size_t i{1u};i < htemp.size();i++)
        {
            lp[0] = Lerp(0.0, lp[0], b);
            lp[1] = Lerp(lp[0], lp[1], b);
            lp[2] = Lerp(lp[1], lp[2], b);
            lp[3] = Lerp(lp[2], lp[3], b);
            htemp[i] = lp[3];
        }
        FftForward(static_cast<uint>(htemp.size()), htemp.data());
        std::transform(htemp.cbegin(), htemp.cbegin()+m, filter.begin(),
            [](const complex_d &c) -> double { return std::abs(c); });

        for(uint ti{0u};ti < channels;ti++)
        {
            for(uint i{0u};i < m;i++)
                field.mEvs[0].mAzs[0].mIrs[ti][i] *= filter[i];
        }
    };
    std::for_each(hData->mFds.begin(), hData->mFds.begin()+hData->mFdCount, proc_field);
}

// The following routines assume a full set of HRIRs for all elevations.

/* Perform minimum-phase reconstruction using the magnitude responses of the
 * HRIR set. Work is delegated to this struct, which runs asynchronously on one
 * or more threads (sharing the same reconstructor object).
 */
struct HrirReconstructor {
    std::vector<double*> mIrs;
    std::atomic<size_t> mCurrent;
    std::atomic<size_t> mDone;
    uint mFftSize;
    uint mIrPoints;

    void Worker()
    {
        auto h = std::vector<complex_d>(mFftSize);
        auto mags = std::vector<double>(mFftSize);
        size_t m{(mFftSize/2) + 1};

        while(1)
        {
            /* Load the current index to process. */
            size_t idx{mCurrent.load()};
            do {
                /* If the index is at the end, we're done. */
                if(idx >= mIrs.size())
                    return;
                /* Otherwise, increment the current index atomically so other
                 * threads know to go to the next one. If this call fails, the
                 * current index was just changed by another thread and the new
                 * value is loaded into idx, which we'll recheck.
                 */
            } while(!mCurrent.compare_exchange_weak(idx, idx+1, std::memory_order_relaxed));

            /* Now do the reconstruction, and apply the inverse FFT to get the
             * time-domain response.
             */
            for(size_t i{0};i < m;++i)
                mags[i] = std::max(mIrs[idx][i], EPSILON);
            MinimumPhase(mFftSize, mags.data(), h.data());
            FftInverse(mFftSize, h.data());
            for(uint i{0u};i < mIrPoints;++i)
                mIrs[idx][i] = h[i].real();

            /* Increment the number of IRs done. */
            mDone.fetch_add(1);
        }
    }
};

static void ReconstructHrirs(const HrirDataT *hData, const uint numThreads)
{
    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};

    /* Set up the reconstructor with the needed size info and pointers to the
     * IRs to process.
     */
    HrirReconstructor reconstructor;
    reconstructor.mCurrent.store(0, std::memory_order_relaxed);
    reconstructor.mDone.store(0, std::memory_order_relaxed);
    reconstructor.mFftSize = hData->mFftSize;
    reconstructor.mIrPoints = hData->mIrPoints;
    for(uint fi{0u};fi < hData->mFdCount;fi++)
    {
        const HrirFdT &field = hData->mFds[fi];
        for(uint ei{0};ei < field.mEvCount;ei++)
        {
            const HrirEvT &elev = field.mEvs[ei];
            for(uint ai{0u};ai < elev.mAzCount;ai++)
            {
                const HrirAzT &azd = elev.mAzs[ai];
                for(uint ti{0u};ti < channels;ti++)
                    reconstructor.mIrs.push_back(azd.mIrs[ti]);
            }
        }
    }

    /* Launch threads to work on reconstruction. */
    std::vector<std::thread> thrds;
    thrds.reserve(numThreads);
    for(size_t i{0};i < numThreads;++i)
        thrds.emplace_back(std::mem_fn(&HrirReconstructor::Worker), &reconstructor);

    /* Keep track of the number of IRs done, periodically reporting it. */
    size_t count;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});

        count = reconstructor.mDone.load();
        size_t pcdone{count * 100 / reconstructor.mIrs.size()};

        printf("\r%3zu%% done (%zu of %zu)", pcdone, count, reconstructor.mIrs.size());
        fflush(stdout);
    } while(count < reconstructor.mIrs.size());
    fputc('\n', stdout);

    for(auto &thrd : thrds)
    {
        if(thrd.joinable())
            thrd.join();
    }
}

// Normalize the HRIR set and slightly attenuate the result.
static void NormalizeHrirs(HrirDataT *hData)
{
    const uint channels{(hData->mChannelType == CT_STEREO) ? 2u : 1u};
    const uint irSize{hData->mIrPoints};

    /* Find the maximum amplitude and RMS out of all the IRs. */
    struct LevelPair { double amp, rms; };
    auto proc0_field = [channels,irSize](const LevelPair levels0, const HrirFdT &field) -> LevelPair
    {
        auto proc_elev = [channels,irSize](const LevelPair levels1, const HrirEvT &elev) -> LevelPair
        {
            auto proc_azi = [channels,irSize](const LevelPair levels2, const HrirAzT &azi) -> LevelPair
            {
                auto proc_channel = [irSize](const LevelPair levels3, const double *ir) -> LevelPair
                {
                    /* Calculate the peak amplitude and RMS of this IR. */
                    auto current = std::accumulate(ir, ir+irSize, LevelPair{0.0, 0.0},
                        [](const LevelPair cur, const double impulse) -> LevelPair
                        {
                            return {std::max(std::abs(impulse), cur.amp),
                                cur.rms + impulse*impulse};
                        });
                    current.rms = std::sqrt(current.rms / irSize);

                    /* Accumulate levels by taking the maximum amplitude and RMS. */
                    return LevelPair{std::max(current.amp, levels3.amp),
                        std::max(current.rms, levels3.rms)};
                };
                return std::accumulate(azi.mIrs, azi.mIrs+channels, levels2, proc_channel);
            };
            return std::accumulate(elev.mAzs, elev.mAzs+elev.mAzCount, levels1, proc_azi);
        };
        return std::accumulate(field.mEvs, field.mEvs+field.mEvCount, levels0, proc_elev);
    };
    const auto maxlev = std::accumulate(hData->mFds.begin(), hData->mFds.begin()+hData->mFdCount,
        LevelPair{0.0, 0.0}, proc0_field);

    /* Normalize using the maximum RMS of the HRIRs. The RMS measure for the
     * non-filtered signal is of an impulse with equal length (to the filter):
     *
     * rms_impulse = sqrt(sum([ 1^2, 0^2, 0^2, ... ]) / n)
     *             = sqrt(1 / n)
     *
     * This helps keep a more consistent volume between the non-filtered signal
     * and various data sets.
     */
    double factor{std::sqrt(1.0 / irSize) / maxlev.rms};

    /* Also ensure the samples themselves won't clip. */
    factor = std::min(factor, 0.99/maxlev.amp);

    /* Now scale all IRs by the given factor. */
    auto proc1_field = [channels,irSize,factor](HrirFdT &field) -> void
    {
        auto proc_elev = [channels,irSize,factor](HrirEvT &elev) -> void
        {
            auto proc_azi = [channels,irSize,factor](HrirAzT &azi) -> void
            {
                auto proc_channel = [irSize,factor](double *ir) -> void
                {
                    std::transform(ir, ir+irSize, ir,
                        std::bind(std::multiplies<double>{}, _1, factor));
                };
                std::for_each(azi.mIrs, azi.mIrs+channels, proc_channel);
            };
            std::for_each(elev.mAzs, elev.mAzs+elev.mAzCount, proc_azi);
        };
        std::for_each(field.mEvs, field.mEvs+field.mEvCount, proc_elev);
    };
    std::for_each(hData->mFds.begin(), hData->mFds.begin()+hData->mFdCount, proc1_field);
}

// Calculate the left-ear time delay using a spherical head model.
static double CalcLTD(const double ev, const double az, const double rad, const double dist)
{
    double azp, dlp, l, al;

    azp = std::asin(std::cos(ev) * std::sin(az));
    dlp = std::sqrt((dist*dist) + (rad*rad) + (2.0*dist*rad*sin(azp)));
    l = std::sqrt((dist*dist) - (rad*rad));
    al = (0.5 * M_PI) + azp;
    if(dlp > l)
        dlp = l + (rad * (al - std::acos(rad / dist)));
    return dlp / 343.3;
}

// Calculate the effective head-related time delays for each minimum-phase
// HRIR. This is done per-field since distance delay is ignored.
static void CalculateHrtds(const HeadModelT model, const double radius, HrirDataT *hData)
{
    uint channels = (hData->mChannelType == CT_STEREO) ? 2 : 1;
    double customRatio{radius / hData->mRadius};
    uint ti, fi, ei, ai;

    if(model == HM_SPHERE)
    {
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
            {
                HrirEvT *evd = &hData->mFds[fi].mEvs[ei];

                for(ai = 0;ai < evd->mAzCount;ai++)
                {
                    HrirAzT *azd = &evd->mAzs[ai];

                    for(ti = 0;ti < channels;ti++)
                        azd->mDelays[ti] = CalcLTD(evd->mElevation, azd->mAzimuth, radius, hData->mFds[fi].mDistance);
                }
            }
        }
    }
    else if(customRatio != 1.0)
    {
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
            {
                HrirEvT *evd = &hData->mFds[fi].mEvs[ei];

                for(ai = 0;ai < evd->mAzCount;ai++)
                {
                    HrirAzT *azd = &evd->mAzs[ai];
                    for(ti = 0;ti < channels;ti++)
                        azd->mDelays[ti] *= customRatio;
                }
            }
        }
    }

    double maxHrtd{0.0};
    for(fi = 0;fi < hData->mFdCount;fi++)
    {
        double minHrtd{std::numeric_limits<double>::infinity()};
        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

                for(ti = 0;ti < channels;ti++)
                    minHrtd = std::min(azd->mDelays[ti], minHrtd);
            }
        }

        for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
        {
            for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
            {
                HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];

                for(ti = 0;ti < channels;ti++)
                {
                    azd->mDelays[ti] = (azd->mDelays[ti]-minHrtd) * hData->mIrRate;
                    maxHrtd = std::max(maxHrtd, azd->mDelays[ti]);
                }
            }
        }
    }
    if(maxHrtd > MAX_HRTD)
    {
        fprintf(stdout, "  Scaling for max delay of %f samples to %f\n...\n", maxHrtd, MAX_HRTD);
        const double scale{MAX_HRTD / maxHrtd};
        for(fi = 0;fi < hData->mFdCount;fi++)
        {
            for(ei = 0;ei < hData->mFds[fi].mEvCount;ei++)
            {
                for(ai = 0;ai < hData->mFds[fi].mEvs[ei].mAzCount;ai++)
                {
                    HrirAzT *azd = &hData->mFds[fi].mEvs[ei].mAzs[ai];
                    for(ti = 0;ti < channels;ti++)
                        azd->mDelays[ti] *= scale;
                }
            }
        }
    }
}

// Allocate and configure dynamic HRIR structures.
int PrepareHrirData(const uint fdCount, const double (&distances)[MAX_FD_COUNT],
    const uint (&evCounts)[MAX_FD_COUNT], const uint azCounts[MAX_FD_COUNT * MAX_EV_COUNT],
    HrirDataT *hData)
{
    uint evTotal = 0, azTotal = 0, fi, ei, ai;

    for(fi = 0;fi < fdCount;fi++)
    {
        evTotal += evCounts[fi];
        for(ei = 0;ei < evCounts[fi];ei++)
            azTotal += azCounts[(fi * MAX_EV_COUNT) + ei];
    }
    if(!fdCount || !evTotal || !azTotal)
        return 0;

    hData->mEvsBase.resize(evTotal);
    hData->mAzsBase.resize(azTotal);
    hData->mFds.resize(fdCount);
    hData->mIrCount = azTotal;
    hData->mFdCount = fdCount;
    evTotal = 0;
    azTotal = 0;
    for(fi = 0;fi < fdCount;fi++)
    {
        hData->mFds[fi].mDistance = distances[fi];
        hData->mFds[fi].mEvCount = evCounts[fi];
        hData->mFds[fi].mEvStart = 0;
        hData->mFds[fi].mEvs = &hData->mEvsBase[evTotal];
        evTotal += evCounts[fi];
        for(ei = 0;ei < evCounts[fi];ei++)
        {
            uint azCount = azCounts[(fi * MAX_EV_COUNT) + ei];

            hData->mFds[fi].mIrCount += azCount;
            hData->mFds[fi].mEvs[ei].mElevation = -M_PI / 2.0 + M_PI * ei / (evCounts[fi] - 1);
            hData->mFds[fi].mEvs[ei].mIrCount += azCount;
            hData->mFds[fi].mEvs[ei].mAzCount = azCount;
            hData->mFds[fi].mEvs[ei].mAzs = &hData->mAzsBase[azTotal];
            for(ai = 0;ai < azCount;ai++)
            {
                hData->mFds[fi].mEvs[ei].mAzs[ai].mAzimuth = 2.0 * M_PI * ai / azCount;
                hData->mFds[fi].mEvs[ei].mAzs[ai].mIndex = azTotal + ai;
                hData->mFds[fi].mEvs[ei].mAzs[ai].mDelays[0] = 0.0;
                hData->mFds[fi].mEvs[ei].mAzs[ai].mDelays[1] = 0.0;
                hData->mFds[fi].mEvs[ei].mAzs[ai].mIrs[0] = nullptr;
                hData->mFds[fi].mEvs[ei].mAzs[ai].mIrs[1] = nullptr;
            }
            azTotal += azCount;
        }
    }
    return 1;
}


/* Parse the data set definition and process the source data, storing the
 * resulting data set as desired.  If the input name is NULL it will read
 * from standard input.
 */
static int ProcessDefinition(const char *inName, const uint outRate, const ChannelModeT chanMode,
    const bool farfield, const uint numThreads, const uint fftSize, const int equalize,
    const int surface, const double limit, const uint truncSize, const HeadModelT model,
    const double radius, const char *outName)
{
    HrirDataT hData;

    fprintf(stdout, "Using %u thread%s.\n", numThreads, (numThreads==1)?"":"s");
    if(!inName)
    {
        inName = "stdin";
        fprintf(stdout, "Reading HRIR definition from %s...\n", inName);
        if(!LoadDefInput(std::cin, nullptr, 0, inName, fftSize, truncSize, chanMode, &hData))
            return 0;
    }
    else
    {
        std::unique_ptr<al::ifstream> input{new al::ifstream{inName}};
        if(!input->is_open())
        {
            fprintf(stderr, "Error: Could not open input file '%s'\n", inName);
            return 0;
        }

        char startbytes[4]{};
        input->read(startbytes, sizeof(startbytes));
        std::streamsize startbytecount{input->gcount()};
        if(startbytecount != sizeof(startbytes) || !input->good())
        {
            fprintf(stderr, "Error: Could not read input file '%s'\n", inName);
            return 0;
        }

        if(startbytes[0] == '\x89' && startbytes[1] == 'H' && startbytes[2] == 'D'
            && startbytes[3] == 'F')
        {
            input = nullptr;
            fprintf(stdout, "Reading HRTF data from %s...\n", inName);
            if(!LoadSofaFile(inName, numThreads, fftSize, truncSize, chanMode, &hData))
                return 0;
        }
        else
        {
            fprintf(stdout, "Reading HRIR definition from %s...\n", inName);
            if(!LoadDefInput(*input, startbytes, startbytecount, inName, fftSize, truncSize, chanMode, &hData))
                return 0;
        }
    }

    if(equalize)
    {
        uint c{(hData.mChannelType == CT_STEREO) ? 2u : 1u};
        uint m{hData.mFftSize/2u + 1u};
        auto dfa = std::vector<double>(c * m);

        if(hData.mFdCount > 1)
        {
            fprintf(stdout, "Balancing field magnitudes...\n");
            BalanceFieldMagnitudes(&hData, c, m);
        }
        fprintf(stdout, "Calculating diffuse-field average...\n");
        CalculateDiffuseFieldAverage(&hData, c, m, surface, limit, dfa.data());
        fprintf(stdout, "Performing diffuse-field equalization...\n");
        DiffuseFieldEqualize(c, m, dfa.data(), &hData);
    }
    if(hData.mFds.size() > 1)
    {
        fprintf(stdout, "Sorting %zu fields...\n", hData.mFds.size());
        std::sort(hData.mFds.begin(), hData.mFds.end(),
            [](const HrirFdT &lhs, const HrirFdT &rhs) noexcept
            { return lhs.mDistance < rhs.mDistance; });
        if(farfield)
        {
            fprintf(stdout, "Clearing %zu near field%s...\n", hData.mFds.size()-1,
                (hData.mFds.size()-1 != 1) ? "s" : "");
            hData.mFds.erase(hData.mFds.cbegin(), hData.mFds.cend()-1);
            hData.mFdCount = 1;
        }
    }
    if(outRate != 0 && outRate != hData.mIrRate)
    {
        fprintf(stdout, "Resampling HRIRs...\n");
        ResampleHrirs(outRate, &hData);
    }
    fprintf(stdout, "Synthesizing missing elevations...\n");
    if(model == HM_DATASET)
        SynthesizeOnsets(&hData);
    SynthesizeHrirs(&hData);
    fprintf(stdout, "Performing minimum phase reconstruction...\n");
    ReconstructHrirs(&hData, numThreads);
    fprintf(stdout, "Truncating minimum-phase HRIRs...\n");
    hData.mIrPoints = truncSize;
    fprintf(stdout, "Normalizing final HRIRs...\n");
    NormalizeHrirs(&hData);
    fprintf(stdout, "Calculating impulse delays...\n");
    CalculateHrtds(model, (radius > DEFAULT_CUSTOM_RADIUS) ? radius : hData.mRadius, &hData);

    const auto rateStr = std::to_string(hData.mIrRate);
    const auto expName = StrSubst({outName, strlen(outName)}, {"%r", 2},
        {rateStr.data(), rateStr.size()});
    fprintf(stdout, "Creating MHR data set %s...\n", expName.c_str());
    return StoreMhr(&hData, expName.c_str());
}

static void PrintHelp(const char *argv0, FILE *ofile)
{
    fprintf(ofile, "Usage:  %s [<option>...]\n\n", argv0);
    fprintf(ofile, "Options:\n");
    fprintf(ofile, " -r <rate>       Change the data set sample rate to the specified value and\n");
    fprintf(ofile, "                 resample the HRIRs accordingly.\n");
    fprintf(ofile, " -m              Change the data set to mono, mirroring the left ear for the\n");
    fprintf(ofile, "                 right ear.\n");
    fprintf(ofile, " -a              Change the data set to single field, using the farthest field.\n");
    fprintf(ofile, " -j <threads>    Number of threads used to process HRIRs (default: 2).\n");
    fprintf(ofile, " -f <points>     Override the FFT window size (default: %u).\n", DEFAULT_FFTSIZE);
    fprintf(ofile, " -e {on|off}     Toggle diffuse-field equalization (default: %s).\n", (DEFAULT_EQUALIZE ? "on" : "off"));
    fprintf(ofile, " -s {on|off}     Toggle surface-weighted diffuse-field average (default: %s).\n", (DEFAULT_SURFACE ? "on" : "off"));
    fprintf(ofile, " -l {<dB>|none}  Specify a limit to the magnitude range of the diffuse-field\n");
    fprintf(ofile, "                 average (default: %.2f).\n", DEFAULT_LIMIT);
    fprintf(ofile, " -w <points>     Specify the size of the truncation window that's applied\n");
    fprintf(ofile, "                 after minimum-phase reconstruction (default: %u).\n", DEFAULT_TRUNCSIZE);
    fprintf(ofile, " -d {dataset|    Specify the model used for calculating the head-delay timing\n");
    fprintf(ofile, "     sphere}     values (default: %s).\n", ((DEFAULT_HEAD_MODEL == HM_DATASET) ? "dataset" : "sphere"));
    fprintf(ofile, " -c <radius>     Use a customized head radius measured to-ear in meters.\n");
    fprintf(ofile, " -i <filename>   Specify an HRIR definition file to use (defaults to stdin).\n");
    fprintf(ofile, " -o <filename>   Specify an output file. Use of '%%r' will be substituted with\n");
    fprintf(ofile, "                 the data set sample rate.\n");
}

// Standard command line dispatch.
int main(int argc, char *argv[])
{
    const char *inName = nullptr, *outName = nullptr;
    uint outRate, fftSize;
    int equalize, surface;
    char *end = nullptr;
    ChannelModeT chanMode;
    HeadModelT model;
    uint numThreads;
    uint truncSize;
    double radius;
    bool farfield;
    double limit;
    int opt;

    if(argc < 2)
    {
        fprintf(stdout, "HRTF Processing and Composition Utility\n\n");
        PrintHelp(argv[0], stdout);
        exit(EXIT_SUCCESS);
    }

    outName = "./oalsoft_hrtf_%r.mhr";
    outRate = 0;
    chanMode = CM_AllowStereo;
    fftSize = DEFAULT_FFTSIZE;
    equalize = DEFAULT_EQUALIZE;
    surface = DEFAULT_SURFACE;
    limit = DEFAULT_LIMIT;
    numThreads = 2;
    truncSize = DEFAULT_TRUNCSIZE;
    model = DEFAULT_HEAD_MODEL;
    radius = DEFAULT_CUSTOM_RADIUS;
    farfield = false;

    while((opt=getopt(argc, argv, "r:maj:f:e:s:l:w:d:c:e:i:o:h")) != -1)
    {
        switch(opt)
        {
        case 'r':
            outRate = static_cast<uint>(strtoul(optarg, &end, 10));
            if(end[0] != '\0' || outRate < MIN_RATE || outRate > MAX_RATE)
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected between %u to %u.\n", optarg, opt, MIN_RATE, MAX_RATE);
                exit(EXIT_FAILURE);
            }
            break;

        case 'm':
            chanMode = CM_ForceMono;
            break;

        case 'a':
            farfield = true;
            break;

        case 'j':
            numThreads = static_cast<uint>(strtoul(optarg, &end, 10));
            if(end[0] != '\0' || numThreads > 64)
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected between %u to %u.\n", optarg, opt, 0, 64);
                exit(EXIT_FAILURE);
            }
            if(numThreads == 0)
                numThreads = std::thread::hardware_concurrency();
            break;

        case 'f':
            fftSize = static_cast<uint>(strtoul(optarg, &end, 10));
            if(end[0] != '\0' || (fftSize&(fftSize-1)) || fftSize < MIN_FFTSIZE || fftSize > MAX_FFTSIZE)
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected a power-of-two between %u to %u.\n", optarg, opt, MIN_FFTSIZE, MAX_FFTSIZE);
                exit(EXIT_FAILURE);
            }
            break;

        case 'e':
            if(strcmp(optarg, "on") == 0)
                equalize = 1;
            else if(strcmp(optarg, "off") == 0)
                equalize = 0;
            else
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected on or off.\n", optarg, opt);
                exit(EXIT_FAILURE);
            }
            break;

        case 's':
            if(strcmp(optarg, "on") == 0)
                surface = 1;
            else if(strcmp(optarg, "off") == 0)
                surface = 0;
            else
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected on or off.\n", optarg, opt);
                exit(EXIT_FAILURE);
            }
            break;

        case 'l':
            if(strcmp(optarg, "none") == 0)
                limit = 0.0;
            else
            {
                limit = strtod(optarg, &end);
                if(end[0] != '\0' || limit < MIN_LIMIT || limit > MAX_LIMIT)
                {
                    fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected between %.0f to %.0f.\n", optarg, opt, MIN_LIMIT, MAX_LIMIT);
                    exit(EXIT_FAILURE);
                }
            }
            break;

        case 'w':
            truncSize = static_cast<uint>(strtoul(optarg, &end, 10));
            if(end[0] != '\0' || truncSize < MIN_TRUNCSIZE || truncSize > MAX_TRUNCSIZE)
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected between %u to %u.\n", optarg, opt, MIN_TRUNCSIZE, MAX_TRUNCSIZE);
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            if(strcmp(optarg, "dataset") == 0)
                model = HM_DATASET;
            else if(strcmp(optarg, "sphere") == 0)
                model = HM_SPHERE;
            else
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected dataset or sphere.\n", optarg, opt);
                exit(EXIT_FAILURE);
            }
            break;

        case 'c':
            radius = strtod(optarg, &end);
            if(end[0] != '\0' || radius < MIN_CUSTOM_RADIUS || radius > MAX_CUSTOM_RADIUS)
            {
                fprintf(stderr, "\nError: Got unexpected value \"%s\" for option -%c, expected between %.2f to %.2f.\n", optarg, opt, MIN_CUSTOM_RADIUS, MAX_CUSTOM_RADIUS);
                exit(EXIT_FAILURE);
            }
            break;

        case 'i':
            inName = optarg;
            break;

        case 'o':
            outName = optarg;
            break;

        case 'h':
            PrintHelp(argv[0], stdout);
            exit(EXIT_SUCCESS);

        default: /* '?' */
            PrintHelp(argv[0], stderr);
            exit(EXIT_FAILURE);
        }
    }

    int ret = ProcessDefinition(inName, outRate, chanMode, farfield, numThreads, fftSize, equalize,
        surface, limit, truncSize, model, radius, outName);
    if(!ret) return -1;
    fprintf(stdout, "Operation completed.\n");

    return EXIT_SUCCESS;
}
