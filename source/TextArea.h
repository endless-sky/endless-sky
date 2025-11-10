/* TextArea.h
Copyright (c) 2024 by thewierdnut

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
#include "ScrollBar.h"
#include "ScrollVar.h"
#include "text/WrappedText.h"

#include <memory>

class Font;
class Rectangle;
class RenderBuffer;
class WrappedText;



// Represents a rect on the screen that needs to display text. The text can be
// larger than the display area, in which case the class will allow the text
// to scroll in response to use input.
class TextArea : public Panel
{
public:
	TextArea();
	explicit TextArea(const Rectangle &r);
	virtual ~TextArea();
	void SetText(const std::string &s);

	void SetRect(const Rectangle &r);
	void SetFont(const Font &f);
	void SetColor(const Color &c);
	void SetAlignment(Alignment a);
	void SetTruncate(Truncate t);

	int GetTextHeight(bool trailingBreak = true);
	int GetLongestLineWidth();


protected:
	virtual void Draw() override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Hover(int x, int y) override;
	virtual bool Scroll(double dx, double dy) override;

	void Invalidate();
	void Validate(bool trailingBreak);


private:
	bool bufferIsValid = false;
	bool textIsValid = false;
	std::shared_ptr<RenderBuffer> buffer;
	WrappedText wrappedText;
	std::string text;
	Color color;
	Point position;
	Point size;

	ScrollVar<double> scroll;
	bool dragging = false;
	bool hovering = false;

	ScrollBar scrollBar;
	bool scrollHeightIncludesTrailingBreak = false;
};
