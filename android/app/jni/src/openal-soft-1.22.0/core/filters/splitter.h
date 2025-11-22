#ifndef CORE_FILTERS_SPLITTER_H
#define CORE_FILTERS_SPLITTER_H

#include <cstddef>

#include "alspan.h"


/* Band splitter. Splits a signal into two phase-matching frequency bands. */
template<typename Real>
class BandSplitterR {
    Real mCoeff{0.0f};
    Real mLpZ1{0.0f};
    Real mLpZ2{0.0f};
    Real mApZ1{0.0f};

public:
    BandSplitterR() = default;
    BandSplitterR(const BandSplitterR&) = default;
    BandSplitterR(Real f0norm) { init(f0norm); }
    BandSplitterR& operator=(const BandSplitterR&) = default;

    void init(Real f0norm);
    void clear() noexcept { mLpZ1 = mLpZ2 = mApZ1 = 0.0f; }
    void process(const al::span<const Real> input, Real *hpout, Real *lpout);

    void processHfScale(const al::span<const Real> input, Real *output, const Real hfscale);

    void processHfScale(const al::span<Real> samples, const Real hfscale);
    void processScale(const al::span<Real> samples, const Real hfscale, const Real lfscale);

    /**
     * The all-pass portion of the band splitter. Applies the same phase shift
     * without splitting the signal, in reverse. It starts from the back of the
     * span and works toward the front, creating a phase shift of -n degrees
     * instead of +n. Note that each use of this method is indepedent, it does
     * not track history between calls.
     */
    void applyAllpassRev(const al::span<Real> samples) const;
};
using BandSplitter = BandSplitterR<float>;

#endif /* CORE_FILTERS_SPLITTER_H */
