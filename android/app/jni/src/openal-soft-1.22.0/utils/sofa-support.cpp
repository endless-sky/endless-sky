/*
 * SOFA utility methods for inspecting SOFA file metrics and determining HRTF
 * utility compatible layouts.
 *
 * Copyright (C) 2018-2019  Christopher Fitzgerald
 * Copyright (C) 2019  Christopher Robinson
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
 */

#include "sofa-support.h"

#include <stdio.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

#include "mysofa.h"


namespace {

using uint = unsigned int;
using double3 = std::array<double,3>;


/* Produces a sorted array of unique elements from a particular axis of the
 * triplets array.  The filters are used to focus on particular coordinates
 * of other axes as necessary.  The epsilons are used to constrain the
 * equality of unique elements.
 */
std::vector<double> GetUniquelySortedElems(const std::vector<double3> &aers, const uint axis,
    const double *const (&filters)[3], const double (&epsilons)[3])
{
    std::vector<double> elems;
    for(const double3 &aer : aers)
    {
        const double elem{aer[axis]};

        uint j;
        for(j = 0;j < 3;j++)
        {
            if(filters[j] && std::abs(aer[j] - *filters[j]) > epsilons[j])
                break;
        }
        if(j < 3)
            continue;

        auto iter = elems.begin();
        for(;iter != elems.end();++iter)
        {
            const double delta{elem - *iter};
            if(delta > epsilons[axis]) continue;
            if(delta >= -epsilons[axis]) break;

            iter = elems.emplace(iter, elem);
            break;
        }
        if(iter == elems.end())
            elems.emplace_back(elem);
    }
    return elems;
}

/* Given a list of azimuths, this will produce the smallest step size that can
 * uniformly cover the list. Ideally this will be over half, but in degenerate
 * cases this can fall to a minimum of 5 (the lower limit).
 */
double GetUniformAzimStep(const double epsilon, const std::vector<double> &elems)
{
    if(elems.size() < 5) return 0.0;

    /* Get the maximum count possible, given the first two elements. It would
     * be impossible to have more than this since the first element must be
     * included.
     */
    uint count{static_cast<uint>(std::ceil(360.0 / (elems[1]-elems[0])))};
    count = std::min(count, 255u);

    for(;count >= 5;--count)
    {
        /* Given the stepping value for this number of elements, check each
         * multiple to ensure there's a matching element.
         */
        const double step{360.0 / count};
        bool good{true};
        size_t idx{1u};
        for(uint mult{1u};mult < count && good;++mult)
        {
            const double target{step*mult + elems[0]};
            while(idx < elems.size() && target-elems[idx] > epsilon)
                ++idx;
            good &= (idx < elems.size()) && !(std::abs(target-elems[idx++]) > epsilon);
        }
        if(good)
            return step;
    }
    return 0.0;
}

/* Given a list of elevations, this will produce the smallest step size that
 * can uniformly cover the list. Ideally this will be over half, but in
 * degenerate cases this can fall to a minimum of 5 (the lower limit).
 */
double GetUniformElevStep(const double epsilon, std::vector<double> &elems)
{
    if(elems.size() < 5) return 0.0;

    /* Reverse the elevations so it increments starting with -90 (flipped from
     * +90). This makes it easier to work out a proper stepping value.
     */
    std::reverse(elems.begin(), elems.end());
    for(auto &v : elems) v *= -1.0;

    uint count{static_cast<uint>(std::ceil(180.0 / (elems[1]-elems[0])))};
    count = std::min(count, 255u);

    double ret{0.0};
    for(;count >= 5;--count)
    {
        const double step{180.0 / count};
        bool good{true};
        size_t idx{1u};
        /* Elevations don't need to match all multiples if there's not enough
         * elements to check. Missing elevations can be synthesized.
         */
        for(uint mult{1u};mult <= count && idx < elems.size() && good;++mult)
        {
            const double target{step*mult + elems[0]};
            while(idx < elems.size() && target-elems[idx] > epsilon)
                ++idx;
            good &= !(idx < elems.size()) || !(std::abs(target-elems[idx++]) > epsilon);
        }
        if(good)
        {
            ret = step;
            break;
        }
    }
    /* Re-reverse the elevations to restore the correct order. */
    for(auto &v : elems) v *= -1.0;
    std::reverse(elems.begin(), elems.end());

    return ret;
}

} // namespace


const char *SofaErrorStr(int err)
{
    switch(err)
    {
    case MYSOFA_OK: return "OK";
    case MYSOFA_INVALID_FORMAT: return "Invalid format";
    case MYSOFA_UNSUPPORTED_FORMAT: return "Unsupported format";
    case MYSOFA_INTERNAL_ERROR: return "Internal error";
    case MYSOFA_NO_MEMORY: return "Out of memory";
    case MYSOFA_READ_ERROR: return "Read error";
    }
    return "Unknown";
}

std::vector<SofaField> GetCompatibleLayout(const size_t m, const float *xyzs)
{
    auto aers = std::vector<double3>(m, double3{});
    for(size_t i{0u};i < m;++i)
    {
        float vals[3]{xyzs[i*3], xyzs[i*3 + 1], xyzs[i*3 + 2]};
        mysofa_c2s(&vals[0]);
        aers[i] = {vals[0], vals[1], vals[2]};
    }

    auto radii = GetUniquelySortedElems(aers, 2, {}, {0.1, 0.1, 0.001});
    std::vector<SofaField> fds;
    fds.reserve(radii.size());

    for(const double dist : radii)
    {
        auto elevs = GetUniquelySortedElems(aers, 1, {nullptr, nullptr, &dist}, {0.1, 0.1, 0.001});

        /* Remove elevations that don't have a valid set of azimuths. */
        auto invalid_elev = [&dist,&aers](const double ev) -> bool
        {
            auto azims = GetUniquelySortedElems(aers, 0, {nullptr, &ev, &dist}, {0.1, 0.1, 0.001});

            if(std::abs(ev) > 89.999)
                return azims.size() != 1;
            if(azims.empty() || !(std::abs(azims[0]) < 0.1))
                return true;
            return GetUniformAzimStep(0.1, azims) <= 0.0;
        };
        elevs.erase(std::remove_if(elevs.begin(), elevs.end(), invalid_elev), elevs.end());

        double step{GetUniformElevStep(0.1, elevs)};
        if(step <= 0.0)
        {
            if(elevs.empty())
                fprintf(stdout, "No usable elevations on field distance %f.\n", dist);
            else
            {
                fprintf(stdout, "Non-uniform elevations on field distance %.3f.\nGot: %+.2f", dist,
                    elevs[0]);
                for(size_t ei{1u};ei < elevs.size();++ei)
                    fprintf(stdout, ", %+.2f", elevs[ei]);
                fputc('\n', stdout);
            }
            continue;
        }

        uint evStart{0u};
        for(uint ei{0u};ei < elevs.size();ei++)
        {
            if(!(elevs[ei] < 0.0))
            {
                fprintf(stdout, "Too many missing elevations on field distance %f.\n", dist);
                return fds;
            }

            double eif{(90.0+elevs[ei]) / step};
            const double ev_start{std::round(eif)};

            if(std::abs(eif - ev_start) < (0.1/step))
            {
                evStart = static_cast<uint>(ev_start);
                break;
            }
        }

        const auto evCount = static_cast<uint>(std::round(180.0 / step)) + 1;
        if(evCount < 5)
        {
            fprintf(stdout, "Too few uniform elevations on field distance %f.\n", dist);
            continue;
        }

        SofaField field{};
        field.mDistance = dist;
        field.mEvCount = evCount;
        field.mEvStart = evStart;
        field.mAzCounts.resize(evCount, 0u);
        auto &azCounts = field.mAzCounts;

        for(uint ei{evStart};ei < evCount;ei++)
        {
            double ev{-90.0 + ei*180.0/(evCount - 1)};
            auto azims = GetUniquelySortedElems(aers, 0, {nullptr, &ev, &dist}, {0.1, 0.1, 0.001});

            if(ei == 0 || ei == (evCount-1))
            {
                if(azims.size() != 1)
                {
                    fprintf(stdout, "Non-singular poles on field distance %f.\n", dist);
                    return fds;
                }
                azCounts[ei] = 1;
            }
            else
            {
                step = GetUniformAzimStep(0.1, azims);
                if(step <= 0.0)
                {
                    fprintf(stdout, "Non-uniform azimuths on elevation %f, field distance %f.\n",
                        ev, dist);
                    return fds;
                }
                azCounts[ei] = static_cast<uint>(std::round(360.0f / step));
            }
        }

        fds.emplace_back(std::move(field));
    }

    return fds;
}
