/* ScrollBar.h
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

#pragma once

#include "Color.h"
#include "Panel.h"
#include "ScrollVar.h"



class ScrollBar : public Panel {
public:
	ScrollBar(
		float fraction,
		float displaySizeFraction,
		const Point &from,
		const Point &to,
		float tabWidth,
		float lineWidth,
		Color color,
		Color innerColor
	) noexcept;

	ScrollBar() noexcept;

	void Draw() override;
	void DrawAt(const Point &from);
	bool Hover(int x, int y) override;
	bool Drag(double dx, double dy) override;
	bool Click(int x, int y, int clicks) override;

	template<typename T>
	void SyncFrom(const ScrollVar<T> &scroll, const Point &from, const Point &to, bool animated = true);
	template<typename T>
	void SyncInto(ScrollVar<T> &scroll, int steps = 5);


public:
	float fraction;
	float displaySizeFraction;
	Point from;
	Point to;
	float tabWidth;
	float lineWidth;
	Color color;
	Color innerColor;
	bool highlighted = false;
	bool innerHighlighted = false;
};



template<typename T>
void ScrollBar::SyncFrom(const ScrollVar<T> &scroll, const Point &from, const Point &to, bool animated)
{
	this->displaySizeFraction = scroll.DisplaySize() / scroll.MaxValue();
	this->fraction = animated ? scroll.AnimatedScrollFraction() : scroll.ScrollFraction();
	this->from = from;
	this->to = to;
}



template<typename T>
void ScrollBar::SyncInto(ScrollVar<T> &scroll, int steps)
{
	scroll.Set(fraction * (scroll.MaxValue() - scroll.DisplaySize()), steps);
}
