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



// Helper class for easily creating/handling/drawing scrollbars.
//
// Useable as a panel, with special considerations:
// - Scroll percentage/start/end will have to be manually updated when needed.
// - Changes will have to be checked every frame, as there is no way to notify parents on change.
//
// Otherwise, manually use the functions inside the regular panel update cycle.
//
// If not using as a child panel, it is recommended to use the scrollbar with a `ScrollVar`, and use the
// Sync* versions of the methods to automatically update the `ScrollVar` on changes.
//
// Check existing uses for more in-depth examples.
class ScrollBar : public Panel {
public:
	ScrollBar(float fraction, float displaySizeFraction, const Point &from, const Point &to,
		float tabWidth, float lineWidth, const Color &color, const Color &innerColor) noexcept;

	ScrollBar() noexcept;

	void Draw() override;
	// Draw at a point, overriding the stored 'from' position. Useful for RenderBuffers to be drawn
	// without affecting the input handling.
	void DrawAt(const Point &from);
	bool Hover(int x, int y) override;
	bool Drag(double dx, double dy) override;
	bool Click(int x, int y, MouseButton button, int clicks) override;

	// Match the state of this scrollbar with the state from the ScrollVar.
	template<typename T>
	void SyncFrom(const ScrollVar<T> &scroll, const Point &from, const Point &to, bool animated = true);
	// Match the state of the ScrollVar with the state from this scrollbar.
	template<typename T>
	void SyncInto(ScrollVar<T> &scroll, int steps = 5);
	// Draw a scrollbar with a start, end, and state, syncing automatically.
	template<typename T>
	void SyncDraw(const ScrollVar<T> &scroll, const Point &from, const Point &to, bool animated = true);
	// Handle a click event, automatically syncing into the given ScrollVar if anything changed.
	template<typename T>
	bool SyncClick(ScrollVar<T> &scroll, int x, int y, MouseButton button, int clicks);
	// Handle a drag event, automatically syncing into the given ScrollVar if anything changed.
	template<typename T>
	bool SyncDrag(ScrollVar<T> &scroll, double dx, double dy);


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



template<typename T>
void ScrollBar::SyncDraw(const ScrollVar<T> &scroll, const Point &from, const Point &to, bool animated)
{
	SyncFrom(scroll, from, to);
	Draw();
}



template<typename T>
bool ScrollBar::SyncClick(ScrollVar<T> &scroll, int x, int y, MouseButton button, int clicks)
{
	if(Click(x, y, button, clicks))
	{
		SyncInto(scroll);
		return true;
	}
	return false;
}

template<typename T>
bool ScrollBar::SyncDrag(ScrollVar<T> &scroll, double dx, double dy)
{
	SyncFrom(scroll, from, to, false);
	if(Drag(dx, dy))
	{
		SyncInto(scroll, 0);
		return true;
	}
	return false;
}
