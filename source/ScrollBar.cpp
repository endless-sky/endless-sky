/* ScrollBar.cpp
Copyright (c) 2024 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ScrollBar.h"

#include "LineShader.h"
#include <algorithm>
#include <iostream>

namespace {
    // Additional distance the scrollbar's tab can be selected from.
    constexpr int SCROLLBAR_MOUSE_ADDITIONAL_RANGE = 5;
}

ScrollBar::ScrollBar(
    float fraction,
    float displaySizeFraction,
    const Point &from,
    const Point &to,
    float tabWidth,
    float lineWidth,
    Color color,
    Color innerColor
) noexcept :
    fraction(fraction),
    displaySizeFraction(displaySizeFraction),
    from(from),
    to(to),
    tabWidth(tabWidth),
    lineWidth(lineWidth),
    color(color),
    innerColor(innerColor)
{
}



ScrollBar::ScrollBar() noexcept : ScrollBar(
   	0.0,
   	0.0,
    Point(),
    Point(),
   	3,
    3,
   	Color(0.6),
   	Color(0.25)
)
{
}



void ScrollBar::Draw() {
    DrawAt(from);
}

void ScrollBar::DrawAt(const Point &from)
{
    auto delta = to - this->from;
    LineShader::Draw(
        from,
        from + delta,
        lineWidth,
        innerHighlighted ? innerColor : Color::Multiply(0.5, innerColor)
    );

    auto deltaOffset = delta * displaySizeFraction;
    auto deltap = (delta * (1 - displaySizeFraction));
    auto offset = deltap * fraction;
    LineShader::Draw(
        from + offset + deltaOffset,
        from + offset,
        tabWidth,
        highlighted ? color : Color::Combine(0.5, color, 0.5, innerColor)
    );
}

bool ScrollBar::Hover(int x, int y)
{
    auto delta = to - from;
    auto deltap = (delta * (1.0 - displaySizeFraction));
    auto offset = deltap * fraction;
    auto center = from + offset;

    auto deltam = delta * displaySizeFraction;

    constexpr auto LineSDF = [](auto a, auto b, auto p){
        auto pa = p - a;
        auto ba = b - a;

        auto h = std::clamp(pa.Dot(ba) / ba.LengthSquared(), 0.0, 1.0);
        auto d = (pa - ba * h).Length();

        return d;
    };

    auto a = center + deltam;
    auto b = center;

    auto p = Point(x, y);

    highlighted = LineSDF(a, b, p) <= tabWidth + SCROLLBAR_MOUSE_ADDITIONAL_RANGE;
    innerHighlighted = LineSDF(from, to, p) <= lineWidth + SCROLLBAR_MOUSE_ADDITIONAL_RANGE;

    // std::cout << to << " - " << from << " -> " << p << " => " << LineSDF(from, to, p) << " (" << innerHighlighted << ")\n";
    return false;
}

bool ScrollBar::Drag(double dx, double dy)
{
    if(highlighted) {
        Point dragVector{dx, dy};
        Point thisVector = to - from;

        auto scalarProjectionOverLength = thisVector.Dot(dragVector) / thisVector.LengthSquared();

        fraction += scalarProjectionOverLength / (1.0 - displaySizeFraction);

        return true;
    }
    return false;
}

bool ScrollBar::Click(int x, int y, int clicks)
{
    if(innerHighlighted && !highlighted)
    {
        Point cursorVector = Point(x, y) - from;
        Point thisVector = to - from;

        auto scalarProjectionOverLength = thisVector.Dot(cursorVector) / thisVector.LengthSquared();

        fraction = (scalarProjectionOverLength - 0.5) / (1.0 - displaySizeFraction) + 0.5;

        highlighted = true;
    }

    return highlighted;
}
