#ifndef CORE_AMBDEC_H
#define CORE_AMBDEC_H

#include <array>
#include <memory>
#include <string>

#include "aloptional.h"
#include "core/ambidefs.h"

/* Helpers to read .ambdec configuration files. */

enum class AmbDecScale {
    N3D,
    SN3D,
    FuMa,
};
struct AmbDecConf {
    std::string Description;
    int Version{0}; /* Must be 3 */

    unsigned int ChanMask{0u};
    unsigned int FreqBands{0u}; /* Must be 1 or 2 */
    AmbDecScale CoeffScale{};

    float XOverFreq{0.0f};
    float XOverRatio{0.0f};

    struct SpeakerConf {
        std::string Name;
        float Distance{0.0f};
        float Azimuth{0.0f};
        float Elevation{0.0f};
        std::string Connection;
    };
    size_t NumSpeakers{0};
    std::unique_ptr<SpeakerConf[]> Speakers;

    using CoeffArray = std::array<float,MaxAmbiChannels>;
    std::unique_ptr<CoeffArray[]> Matrix;

    /* Unused when FreqBands == 1 */
    float LFOrderGain[MaxAmbiOrder+1]{};
    CoeffArray *LFMatrix;

    float HFOrderGain[MaxAmbiOrder+1]{};
    CoeffArray *HFMatrix;

    ~AmbDecConf();

    al::optional<std::string> load(const char *fname) noexcept;
};

#endif /* CORE_AMBDEC_H */
