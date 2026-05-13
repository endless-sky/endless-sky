/* ScrollArea.h
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

#pragma once

#include "Panel.h"

#include "Point.h"
#include "ScrollBar.h"
#include "ScrollVar.h"

#include <memory>

class Font;
class Rectangle;
class RenderBuffer;



// A base class for areas on the screen that display scrollable text.
// Subclasses will control how this text is populated and formatted.
class ScrollArea : public Panel
{
public:
	ScrollArea();
	explicit ScrollArea(const Rectangle &r);
	virtual ~ScrollArea() override;

	virtual void SetRect(const Rectangle &r);


protected:
	virtual void Draw() override;
	virtual void DrawText(const Point &topLeft);
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;

	void Invalidate();
	virtual void Validate(bool trailingBreak);


protected:
	bool bufferIsValid = false;
	bool contentsIsValid = false;
	std::shared_ptr<RenderBuffer> buffer;
	Point position;
	Point size;

	ScrollVar<double> scroll;
	bool dragging = false;
	bool hovering = false;

	ScrollBar scrollBar;
	bool scrollHeightIncludesTrailingBreak = false;
};
