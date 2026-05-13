/* ScrollArea.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ScrollArea.h"

#include "GameData.h"
#include "Preferences.h"
#include "RenderBuffer.h"
#include "ScrollBar.h"

using namespace std;



ScrollArea::ScrollArea()
{
	SetInterruptible(true);
	SetTrapAllEvents(false);
	SetIsFullScreen(false);
}



ScrollArea::ScrollArea(const Rectangle &r)
	: ScrollArea()
{
	ScrollArea::SetRect(r);
}



// Stub destructor, so that unique_ptrs are destructed in the correct scope.
ScrollArea::~ScrollArea()
{
}



void ScrollArea::SetRect(const Rectangle &r)
{
	position = r.Center();
	size = r.Dimensions();
	buffer.reset();
	scroll.SetDisplaySize(r.Height());
	scrollBar.displaySizeFraction = scroll.DisplaySize() / scroll.MaxValue();
	Invalidate();
}



void ScrollArea::SetScrollbarOffset(int offset)
{
	scrollbarOffset = offset;
}



void ScrollArea::SetPointerOffset(int offset)
{
	pointerOffset = offset;
}



void ScrollArea::Draw()
{
	if(!buffer)
		buffer = make_unique<RenderBuffer>(size);

	Validate(scrollHeightIncludesTrailingBreak);
	if(!bufferIsValid || !scroll.IsAnimationDone())
	{
		scroll.Step();

		auto target = buffer->SetTarget();
		Point topLeft(buffer->Left(), buffer->Top() - scroll.AnimatedValue());
		DrawText(topLeft);
		target.Deactivate();

		buffer->SetFadePadding(
			scroll.IsScrollAtMin() ? 0 : 20,
			scroll.IsScrollAtMax() ? 0 : 20
		);
		bufferIsValid = true;
	}
	buffer->Draw(position);

	if(scroll.Scrollable())
	{
		Point topRight(position + Point(buffer->Right() + scrollbarOffset, buffer->Top() + pointerOffset));
		Point bottomRight(position + Point(buffer->Right() + scrollbarOffset, buffer->Bottom() - pointerOffset));

		scrollBar.SyncDraw(scroll, topRight, bottomRight);
	}
}



void ScrollArea::DrawText(const Point &topLeft)
{
}



bool ScrollArea::Click(int x, int y, MouseButton button, int clicks)
{
	if(scroll.Scrollable() && scrollBar.SyncClick(scroll, x, y, button, clicks))
	{
		bufferIsValid = false;
		return true;
	}
	if(button != MouseButton::LEFT)
		return false;

	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	dragging = bounds.Contains(Point(x, y));
	return dragging;
}



bool ScrollArea::Drag(double dx, double dy)
{
	if(scrollBar.SyncDrag(scroll, dx, dy))
	{
		bufferIsValid = false;
		return true;
	}
	if(dragging)
	{
		scroll.Scroll(-dy, 0);
		bufferIsValid = false;
		return true;
	}
	return false;
}



bool ScrollArea::Release(int x, int y, MouseButton button)
{
	if(button != MouseButton::LEFT)
		return false;

	bool ret = dragging;
	dragging = false;
	return ret;
}



bool ScrollArea::Hover(int x, int y)
{
	scrollBar.Hover(x, y);

	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	hovering = bounds.Contains(Point(x, y));
	return hovering;
}



bool ScrollArea::Scroll(double dx, double dy)
{
	if(hovering)
	{
		scroll.Scroll(-dy * Preferences::ScrollSpeed());
		bufferIsValid = false;
	}
	return hovering;
}



void ScrollArea::Invalidate()
{
	bufferIsValid = false;
	contentsIsValid = false;
}



void ScrollArea::Validate(bool trailingBreak)
{
}
