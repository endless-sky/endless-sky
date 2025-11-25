#ifndef CORE_FILTERS_BIQUAD_H
#define CORE_FILTERS_BIQUAD_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "alnumbers.h"
#include "alspan.h"


/* Filters implementation is based on the "Cookbook formulae for audio
 * EQ biquad filter coefficients" by Robert Bristow-Johnson
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 */
/* Implementation note: For the shelf and peaking filters, the specified gain
 * is for the centerpoint of the transition band. This better fits EFX filter
 * behavior, which expects the shelf's reference frequency to reach the given
 * gain. To set the gain for the shelf or peak itself, use the square root of
 * the desired linear gain (or halve the dB gain).
 */

enum class BiquadType {
    /** EFX-style low-pass filter, specifying a gain and reference frequency. */
    HighShelf,
    /** EFX-style high-pass filter, specifying a gain and reference frequency. */
    LowShelf,
    /** Peaking filter, specifying a gain and reference frequency. */
    Peaking,

    /** Low-pass cut-off filter, specifying a cut-off frequency. */
    LowPass,
    /** High-pass cut-off filter, specifying a cut-off frequency. */
    HighPass,
    /** Band-pass filter, specifying a center frequency. */
    BandPass,
};

template<typename Real>
class BiquadFilterR {
    /* Last two delayed components for direct form II. */
    Real mZ1{0}, mZ2{0};
    /* Transfer function coefficients "b" (numerator) */
    Real mB0{1}, mB1{0}, mB2{0};
    /* Transfer function coefficients "a" (denominator; a0 is pre-applied). */
    Real mA1{0}, mA2{0};

    void setParams(BiquadType type, Real f0norm, Real gain, Real rcpQ);

    /**
     * Calculates the rcpQ (i.e. 1/Q) coefficient for shelving filters, using
     * the reference gain and shelf slope parameter.
     * \param gain 0 < gain
     * \param slope 0 < slope <= 1
     */
    static Real rcpQFromSlope(Real gain, Real slope)
    { return std::sqrt((gain + Real{1}/gain)*(Real{1}/slope - Real{1}) + Real{2}); }

    /**
     * Calculates the rcpQ (i.e. 1/Q) coefficient for filters, using the
     * normalized reference frequency and bandwidth.
     * \param f0norm 0 < f0norm < 0.5.
     * \param bandwidth 0 < bandwidth
     */
    static Real rcpQFromBandwidth(Real f0norm, Real bandwidth)
    {
        const Real w0{al::numbers::pi_v<Real>*Real{2} * f0norm};
        return 2.0f*std::sinh(std::log(Real{2})/Real{2}*bandwidth*w0/std::sin(w0));
    }

public:
    void clear() noexcept { mZ1 = mZ2 = Real{0}; }

    /**
     * Sets the filter state for the specified filter type and its parameters.
     *
     * \param type The type of filter to apply.
     * \param f0norm The normalized reference frequency (ref / sample_rate).
     * This is the center point for the Shelf, Peaking, and BandPass filter
     * types, or the cutoff frequency for the LowPass and HighPass filter
     * types.
     * \param gain The gain for the reference frequency response. Only used by
     * the Shelf and Peaking filter types.
     * \param slope Slope steepness of the transition band.
     */
    void setParamsFromSlope(BiquadType type, Real f0norm, Real gain, Real slope)
    {
        gain = std::max<Real>(gain, 0.001f); /* Limit -60dB */
        setParams(type, f0norm, gain, rcpQFromSlope(gain, slope));
    }

    /**
     * Sets the filter state for the specified filter type and its parameters.
     *
     * \param type The type of filter to apply.
     * \param f0norm The normalized reference frequency (ref / sample_rate).
     * This is the center point for the Shelf, Peaking, and BandPass filter
     * types, or the cutoff frequency for the LowPass and HighPass filter
     * types.
     * \param gain The gain for the reference frequency response. Only used by
     * the Shelf and Peaking filter types.
     * \param bandwidth Normalized bandwidth of the transition band.
     */
    void setParamsFromBandwidth(BiquadType type, Real f0norm, Real gain, Real bandwidth)
    { setParams(type, f0norm, gain, rcpQFromBandwidth(f0norm, bandwidth)); }

    void copyParamsFrom(const BiquadFilterR &other)
    {
        mB0 = other.mB0;
        mB1 = other.mB1;
        mB2 = other.mB2;
        mA1 = other.mA1;
        mA2 = other.mA2;
    }

    void process(const al::span<const Real> src, Real *dst);
    /** Processes this filter and the other at the same time. */
    void dualProcess(BiquadFilterR &other, const al::span<const Real> src, Real *dst);

    /* Rather hacky. It's just here to support "manual" processing. */
    std::pair<Real,Real> getComponents() const noexcept { return {mZ1, mZ2}; }
    void setComponents(Real z1, Real z2) noexcept { mZ1 = z1; mZ2 = z2; }
    Real processOne(const Real in, Real &z1, Real &z2) const noexcept
    {
        const Real out{in*mB0 + z1};
        z1 = in*mB1 - out*mA1 + z2;
        z2 = in*mB2 - out*mA2;
        return out;
    }
};

template<typename Real>
struct DualBiquadR {
    BiquadFilterR<Real> &f0, &f1;

    void process(const al::span<const Real> src, Real *dst)
    { f0.dualProcess(f1, src, dst); }
};

using BiquadFilter = BiquadFilterR<float>;
using DualBiquad = DualBiquadR<float>;

#endif /* CORE_FILTERS_BIQUAD_H */
