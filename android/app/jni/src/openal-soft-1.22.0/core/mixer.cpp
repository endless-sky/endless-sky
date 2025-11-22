
#include "config.h"

#include "mixer.h"

#include <cmath>

#include "alnumbers.h"
#include "devformat.h"
#include "device.h"
#include "mixer/defs.h"

struct CTag;


MixerFunc MixSamples{Mix_<CTag>};


std::array<float,MaxAmbiChannels> CalcAmbiCoeffs(const float y, const float z, const float x,
    const float spread)
{
    std::array<float,MaxAmbiChannels> coeffs;

    /* Zeroth-order */
    coeffs[0]  = 1.0f; /* ACN 0 = 1 */
    /* First-order */
    coeffs[1]  = al::numbers::sqrt3_v<float> * y; /* ACN 1 = sqrt(3) * Y */
    coeffs[2]  = al::numbers::sqrt3_v<float> * z; /* ACN 2 = sqrt(3) * Z */
    coeffs[3]  = al::numbers::sqrt3_v<float> * x; /* ACN 3 = sqrt(3) * X */
    /* Second-order */
    const float xx{x*x}, yy{y*y}, zz{z*z}, xy{x*y}, yz{y*z}, xz{x*z};
    coeffs[4]  = 3.872983346f * xy;               /* ACN 4 = sqrt(15) * X * Y */
    coeffs[5]  = 3.872983346f * yz;               /* ACN 5 = sqrt(15) * Y * Z */
    coeffs[6]  = 1.118033989f * (3.0f*zz - 1.0f); /* ACN 6 = sqrt(5)/2 * (3*Z*Z - 1) */
    coeffs[7]  = 3.872983346f * xz;               /* ACN 7 = sqrt(15) * X * Z */
    coeffs[8]  = 1.936491673f * (xx - yy);        /* ACN 8 = sqrt(15)/2 * (X*X - Y*Y) */
    /* Third-order */
    coeffs[9]  =  2.091650066f * (y*(3.0f*xx - yy));   /* ACN  9 = sqrt(35/8) * Y * (3*X*X - Y*Y) */
    coeffs[10] = 10.246950766f * (z*xy);               /* ACN 10 = sqrt(105) * Z * X * Y */
    coeffs[11] =  1.620185175f * (y*(5.0f*zz - 1.0f)); /* ACN 11 = sqrt(21/8) * Y * (5*Z*Z - 1) */
    coeffs[12] =  1.322875656f * (z*(5.0f*zz - 3.0f)); /* ACN 12 = sqrt(7)/2 * Z * (5*Z*Z - 3) */
    coeffs[13] =  1.620185175f * (x*(5.0f*zz - 1.0f)); /* ACN 13 = sqrt(21/8) * X * (5*Z*Z - 1) */
    coeffs[14] =  5.123475383f * (z*(xx - yy));        /* ACN 14 = sqrt(105)/2 * Z * (X*X - Y*Y) */
    coeffs[15] =  2.091650066f * (x*(xx - 3.0f*yy));   /* ACN 15 = sqrt(35/8) * X * (X*X - 3*Y*Y) */
    /* Fourth-order */
    /* ACN 16 = sqrt(35)*3/2 * X * Y * (X*X - Y*Y) */
    /* ACN 17 = sqrt(35/2)*3/2 * (3*X*X - Y*Y) * Y * Z */
    /* ACN 18 = sqrt(5)*3/2 * X * Y * (7*Z*Z - 1) */
    /* ACN 19 = sqrt(5/2)*3/2 * Y * Z * (7*Z*Z - 3)  */
    /* ACN 20 = 3/8 * (35*Z*Z*Z*Z - 30*Z*Z + 3) */
    /* ACN 21 = sqrt(5/2)*3/2 * X * Z * (7*Z*Z - 3) */
    /* ACN 22 = sqrt(5)*3/4 * (X*X - Y*Y) * (7*Z*Z - 1) */
    /* ACN 23 = sqrt(35/2)*3/2 * (X*X - 3*Y*Y) * X * Z */
    /* ACN 24 = sqrt(35)*3/8 * (X*X*X*X - 6*X*X*Y*Y + Y*Y*Y*Y) */

    if(spread > 0.0f)
    {
        /* Implement the spread by using a spherical source that subtends the
         * angle spread. See:
         * http://www.ppsloan.org/publications/StupidSH36.pdf - Appendix A3
         *
         * When adjusted for N3D normalization instead of SN3D, these
         * calculations are:
         *
         * ZH0 = -sqrt(pi) * (-1+ca);
         * ZH1 =  0.5*sqrt(pi) * sa*sa;
         * ZH2 = -0.5*sqrt(pi) * ca*(-1+ca)*(ca+1);
         * ZH3 = -0.125*sqrt(pi) * (-1+ca)*(ca+1)*(5*ca*ca - 1);
         * ZH4 = -0.125*sqrt(pi) * ca*(-1+ca)*(ca+1)*(7*ca*ca - 3);
         * ZH5 = -0.0625*sqrt(pi) * (-1+ca)*(ca+1)*(21*ca*ca*ca*ca - 14*ca*ca + 1);
         *
         * The gain of the source is compensated for size, so that the
         * loudness doesn't depend on the spread. Thus:
         *
         * ZH0 = 1.0f;
         * ZH1 = 0.5f * (ca+1.0f);
         * ZH2 = 0.5f * (ca+1.0f)*ca;
         * ZH3 = 0.125f * (ca+1.0f)*(5.0f*ca*ca - 1.0f);
         * ZH4 = 0.125f * (ca+1.0f)*(7.0f*ca*ca - 3.0f)*ca;
         * ZH5 = 0.0625f * (ca+1.0f)*(21.0f*ca*ca*ca*ca - 14.0f*ca*ca + 1.0f);
         */
        const float ca{std::cos(spread * 0.5f)};
        /* Increase the source volume by up to +3dB for a full spread. */
        const float scale{std::sqrt(1.0f + al::numbers::inv_pi_v<float>/2.0f*spread)};

        const float ZH0_norm{scale};
        const float ZH1_norm{scale * 0.5f * (ca+1.f)};
        const float ZH2_norm{scale * 0.5f * (ca+1.f)*ca};
        const float ZH3_norm{scale * 0.125f * (ca+1.f)*(5.f*ca*ca-1.f)};

        /* Zeroth-order */
        coeffs[0]  *= ZH0_norm;
        /* First-order */
        coeffs[1]  *= ZH1_norm;
        coeffs[2]  *= ZH1_norm;
        coeffs[3]  *= ZH1_norm;
        /* Second-order */
        coeffs[4]  *= ZH2_norm;
        coeffs[5]  *= ZH2_norm;
        coeffs[6]  *= ZH2_norm;
        coeffs[7]  *= ZH2_norm;
        coeffs[8]  *= ZH2_norm;
        /* Third-order */
        coeffs[9]  *= ZH3_norm;
        coeffs[10] *= ZH3_norm;
        coeffs[11] *= ZH3_norm;
        coeffs[12] *= ZH3_norm;
        coeffs[13] *= ZH3_norm;
        coeffs[14] *= ZH3_norm;
        coeffs[15] *= ZH3_norm;
    }

    return coeffs;
}

void ComputePanGains(const MixParams *mix, const float*RESTRICT coeffs, const float ingain,
    const al::span<float,MAX_OUTPUT_CHANNELS> gains)
{
    auto ambimap = mix->AmbiMap.cbegin();

    auto iter = std::transform(ambimap, ambimap+mix->Buffer.size(), gains.begin(),
        [coeffs,ingain](const BFChannelConfig &chanmap) noexcept -> float
        { return chanmap.Scale * coeffs[chanmap.Index] * ingain; }
    );
    std::fill(iter, gains.end(), 0.0f);
}
