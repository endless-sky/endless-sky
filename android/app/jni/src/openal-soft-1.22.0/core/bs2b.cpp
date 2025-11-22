/*-
 * Copyright (c) 2005 Boris Mikhaylov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "alnumbers.h"
#include "bs2b.h"


/* Set up all data. */
static void init(struct bs2b *bs2b)
{
    float Fc_lo, Fc_hi;
    float G_lo, G_hi;
    float x, g;

    switch(bs2b->level)
    {
    case BS2B_LOW_CLEVEL: /* Low crossfeed level */
        Fc_lo = 360.0f;
        Fc_hi = 501.0f;
        G_lo  = 0.398107170553497f;
        G_hi  = 0.205671765275719f;
        break;

    case BS2B_MIDDLE_CLEVEL: /* Middle crossfeed level */
        Fc_lo = 500.0f;
        Fc_hi = 711.0f;
        G_lo  = 0.459726988530872f;
        G_hi  = 0.228208484414988f;
        break;

    case BS2B_HIGH_CLEVEL: /* High crossfeed level (virtual speakers are closer to itself) */
        Fc_lo = 700.0f;
        Fc_hi = 1021.0f;
        G_lo  = 0.530884444230988f;
        G_hi  = 0.250105790667544f;
        break;

    case BS2B_LOW_ECLEVEL: /* Low easy crossfeed level */
        Fc_lo = 360.0f;
        Fc_hi = 494.0f;
        G_lo  = 0.316227766016838f;
        G_hi  = 0.168236228897329f;
        break;

    case BS2B_MIDDLE_ECLEVEL: /* Middle easy crossfeed level */
        Fc_lo = 500.0f;
        Fc_hi = 689.0f;
        G_lo  = 0.354813389233575f;
        G_hi  = 0.187169483835901f;
        break;

    default: /* High easy crossfeed level */
        bs2b->level = BS2B_HIGH_ECLEVEL;

        Fc_lo = 700.0f;
        Fc_hi = 975.0f;
        G_lo  = 0.398107170553497f;
        G_hi  = 0.205671765275719f;
        break;
    } /* switch */

    g = 1.0f / (1.0f - G_hi + G_lo);

    /* $fc = $Fc / $s;
     * $d  = 1 / 2 / pi / $fc;
     * $x  = exp(-1 / $d);
     */
    x           = std::exp(-al::numbers::pi_v<float>*2.0f*Fc_lo/static_cast<float>(bs2b->srate));
    bs2b->b1_lo = x;
    bs2b->a0_lo = G_lo * (1.0f - x) * g;

    x           = std::exp(-al::numbers::pi_v<float>*2.0f*Fc_hi/static_cast<float>(bs2b->srate));
    bs2b->b1_hi = x;
    bs2b->a0_hi = (1.0f - G_hi * (1.0f - x)) * g;
    bs2b->a1_hi = -x * g;
} /* init */


/* Exported functions.
 * See descriptions in "bs2b.h"
 */

void bs2b_set_params(struct bs2b *bs2b, int level, int srate)
{
    if(srate <= 0) srate = 1;

    bs2b->level = level;
    bs2b->srate = srate;
    init(bs2b);
} /* bs2b_set_params */

int bs2b_get_level(struct bs2b *bs2b)
{
    return bs2b->level;
} /* bs2b_get_level */

int bs2b_get_srate(struct bs2b *bs2b)
{
    return bs2b->srate;
} /* bs2b_get_srate */

void bs2b_clear(struct bs2b *bs2b)
{
    std::fill(std::begin(bs2b->history), std::end(bs2b->history), bs2b::t_last_sample{});
} /* bs2b_clear */

void bs2b_cross_feed(struct bs2b *bs2b, float *Left, float *Right, size_t SamplesToDo)
{
    const float a0_lo{bs2b->a0_lo};
    const float b1_lo{bs2b->b1_lo};
    const float a0_hi{bs2b->a0_hi};
    const float a1_hi{bs2b->a1_hi};
    const float b1_hi{bs2b->b1_hi};
    float lsamples[128][2];
    float rsamples[128][2];

    for(size_t base{0};base < SamplesToDo;)
    {
        const size_t todo{std::min<size_t>(128, SamplesToDo-base)};

        /* Process left input */
        float z_lo{bs2b->history[0].lo};
        float z_hi{bs2b->history[0].hi};
        for(size_t i{0};i < todo;i++)
        {
            lsamples[i][0] = a0_lo*Left[i] + z_lo;
            z_lo = b1_lo*lsamples[i][0];

            lsamples[i][1] = a0_hi*Left[i] + z_hi;
            z_hi = a1_hi*Left[i] + b1_hi*lsamples[i][1];
        }
        bs2b->history[0].lo = z_lo;
        bs2b->history[0].hi = z_hi;

        /* Process right input */
        z_lo = bs2b->history[1].lo;
        z_hi = bs2b->history[1].hi;
        for(size_t i{0};i < todo;i++)
        {
            rsamples[i][0] = a0_lo*Right[i] + z_lo;
            z_lo = b1_lo*rsamples[i][0];

            rsamples[i][1] = a0_hi*Right[i] + z_hi;
            z_hi = a1_hi*Right[i] + b1_hi*rsamples[i][1];
        }
        bs2b->history[1].lo = z_lo;
        bs2b->history[1].hi = z_hi;

        /* Crossfeed */
        for(size_t i{0};i < todo;i++)
            *(Left++) = lsamples[i][1] + rsamples[i][0];
        for(size_t i{0};i < todo;i++)
            *(Right++) = rsamples[i][1] + lsamples[i][0];

        base += todo;
    }
} /* bs2b_cross_feed */
