#ifndef MAKEMHR_H
#define MAKEMHR_H

#include <vector>
#include <complex>

#include "polyphase_resampler.h"


// The maximum path length used when processing filenames.
#define MAX_PATH_LEN                 (256)

// The limit to the number of 'distances' listed in the data set definition.
// Must be less than 256
#define MAX_FD_COUNT                 (16)

// The limits to the number of 'elevations' listed in the data set definition.
// Must be less than 256.
#define MIN_EV_COUNT                 (5)
#define MAX_EV_COUNT                 (181)

// The limits for each of the 'azimuths' listed in the data set definition.
// Must be less than 256.
#define MIN_AZ_COUNT                 (1)
#define MAX_AZ_COUNT                 (255)

// The limits for the 'distance' from source to listener for each field in
// the definition file.
#define MIN_DISTANCE                 (0.05)
#define MAX_DISTANCE                 (2.50)

// The limits for the sample 'rate' metric in the data set definition and for
// resampling.
#define MIN_RATE                     (32000)
#define MAX_RATE                     (96000)

// The limits for the HRIR 'points' metric in the data set definition.
#define MIN_POINTS                   (16)
#define MAX_POINTS                   (8192)


using uint = unsigned int;

/* Complex double type. */
using complex_d = std::complex<double>;


enum ChannelModeT : bool {
    CM_AllowStereo = false,
    CM_ForceMono = true
};

// Sample and channel type enum values.
enum SampleTypeT {
    ST_S16 = 0,
    ST_S24 = 1
};

// Certain iterations rely on these integer enum values.
enum ChannelTypeT {
    CT_NONE   = -1,
    CT_MONO   = 0,
    CT_STEREO = 1
};

// Structured HRIR storage for stereo azimuth pairs, elevations, and fields.
struct HrirAzT {
    double mAzimuth{0.0};
    uint mIndex{0u};
    double mDelays[2]{0.0, 0.0};
    double *mIrs[2]{nullptr, nullptr};
};

struct HrirEvT {
    double mElevation{0.0};
    uint mIrCount{0u};
    uint mAzCount{0u};
    HrirAzT *mAzs{nullptr};
};

struct HrirFdT {
    double mDistance{0.0};
    uint mIrCount{0u};
    uint mEvCount{0u};
    uint mEvStart{0u};
    HrirEvT *mEvs{nullptr};
};

// The HRIR metrics and data set used when loading, processing, and storing
// the resulting HRTF.
struct HrirDataT {
    uint mIrRate{0u};
    SampleTypeT mSampleType{ST_S24};
    ChannelTypeT mChannelType{CT_NONE};
    uint mIrPoints{0u};
    uint mFftSize{0u};
    uint mIrSize{0u};
    double mRadius{0.0};
    uint mIrCount{0u};
    uint mFdCount{0u};

    std::vector<double> mHrirsBase;
    std::vector<HrirEvT> mEvsBase;
    std::vector<HrirAzT> mAzsBase;

    std::vector<HrirFdT> mFds;
};


int PrepareHrirData(const uint fdCount, const double (&distances)[MAX_FD_COUNT], const uint (&evCounts)[MAX_FD_COUNT], const uint azCounts[MAX_FD_COUNT * MAX_EV_COUNT], HrirDataT *hData);
void MagnitudeResponse(const uint n, const complex_d *in, double *out);
void FftForward(const uint n, complex_d *inout);
void FftInverse(const uint n, complex_d *inout);


// Performs linear interpolation.
inline double Lerp(const double a, const double b, const double f)
{ return a + f * (b - a); }

#endif /* MAKEMHR_H */
