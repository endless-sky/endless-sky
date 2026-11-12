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

#include "ScrollArea.h"

#include "Color.h"
#include "text/WrappedText.h"

class Font;
class Point;
class Rectangle;



// A ScrollArea that renders text using the WrappedText class.
class TextArea : public ScrollArea
{
public:
	TextArea();
	explicit TextArea(const Rectangle &r);
	virtual ~TextArea() override;

	void SetRect(const Rectangle &r) override;

	void SetText(const std::string &s);
	void SetFont(const Font &f);
	void SetParagraphBreak(int height);
	void SetColor(const Color &c);
	void SetAlignment(Alignment a);
	void SetTruncate(Truncate t);

	int GetTextHeight(bool trailingBreak = true);
	int GetLongestLineWidth();

	virtual void Validate(bool trailingBreak) override;


protected:
	virtual void DrawText(const Point &topLeft) override;


private:
	WrappedText wrappedText;
	std::string text;
	Color color;
};
