/*
 * SOFA info utility for inspecting SOFA file metrics and determining HRTF
 * utility compatible layouts.
 *
 * Copyright (C) 2018-2019  Christopher Fitzgerald
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

#include <stdio.h>

#include <memory>
#include <vector>

#include "sofa-support.h"

#include "mysofa.h"

#include "win_main_utf8.h"

using uint = unsigned int;

static void PrintSofaAttributes(const char *prefix, struct MYSOFA_ATTRIBUTE *attribute)
{
    while(attribute)
    {
        fprintf(stdout, "%s.%s: %s\n", prefix, attribute->name, attribute->value);
        attribute = attribute->next;
    }
}

static void PrintSofaArray(const char *prefix, struct MYSOFA_ARRAY *array)
{
    PrintSofaAttributes(prefix, array->attributes);
    for(uint i{0u};i < array->elements;i++)
        fprintf(stdout, "%s[%u]: %.6f\n", prefix, i, array->values[i]);
}

/* Attempts to produce a compatible layout.  Most data sets tend to be
 * uniform and have the same major axis as used by OpenAL Soft's HRTF model.
 * This will remove outliers and produce a maximally dense layout when
 * possible.  Those sets that contain purely random measurements or use
 * different major axes will fail.
 */
static void PrintCompatibleLayout(const uint m, const float *xyzs)
{
    fputc('\n', stdout);

    auto fds = GetCompatibleLayout(m, xyzs);
    if(fds.empty())
    {
        fprintf(stdout, "No compatible field layouts in SOFA file.\n");
        return;
    }

    uint used_elems{0};
    for(size_t fi{0u};fi < fds.size();++fi)
    {
        for(uint ei{fds[fi].mEvStart};ei < fds[fi].mEvCount;++ei)
            used_elems += fds[fi].mAzCounts[ei];
    }

    fprintf(stdout, "Compatible Layout (%u of %u measurements):\n\ndistance = %.3f", used_elems, m,
        fds[0].mDistance);
    for(size_t fi{1u};fi < fds.size();fi++)
        fprintf(stdout, ", %.3f", fds[fi].mDistance);

    fprintf(stdout, "\nazimuths = ");
    for(size_t fi{0u};fi < fds.size();++fi)
    {
        for(uint ei{0u};ei < fds[fi].mEvStart;++ei)
            fprintf(stdout, "%d%s", fds[fi].mAzCounts[fds[fi].mEvCount - 1 - ei], ", ");
        for(uint ei{fds[fi].mEvStart};ei < fds[fi].mEvCount;++ei)
            fprintf(stdout, "%d%s", fds[fi].mAzCounts[ei],
                (ei < (fds[fi].mEvCount - 1)) ? ", " :
                (fi < (fds.size() - 1)) ? ";\n           " : "\n");
    }
}

// Load and inspect the given SOFA file.
static void SofaInfo(const char *filename)
{
    int err;
    MySofaHrtfPtr sofa{mysofa_load(filename, &err)};
    if(!sofa)
    {
        fprintf(stdout, "Error: Could not load source file '%s' (%s).\n", filename,
            SofaErrorStr(err));
        return;
    }

    /* NOTE: Some valid SOFA files are failing this check. */
    err = mysofa_check(sofa.get());
    if(err != MYSOFA_OK)
        fprintf(stdout, "Warning: Supposedly malformed source file '%s' (%s).\n", filename,
            SofaErrorStr(err));

    mysofa_tocartesian(sofa.get());

    PrintSofaAttributes("Info", sofa->attributes);

    fprintf(stdout, "Measurements: %u\n", sofa->M);
    fprintf(stdout, "Receivers: %u\n", sofa->R);
    fprintf(stdout, "Emitters: %u\n", sofa->E);
    fprintf(stdout, "Samples: %u\n", sofa->N);

    PrintSofaArray("SampleRate", &sofa->DataSamplingRate);
    PrintSofaArray("DataDelay", &sofa->DataDelay);

    PrintCompatibleLayout(sofa->M, sofa->SourcePosition.values);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        fprintf(stdout, "Usage: %s <sofa-file>\n", argv[0]);
        return 0;
    }

    SofaInfo(argv[1]);

    return 0;
}

