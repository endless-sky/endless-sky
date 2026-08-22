/* TextArea.cpp
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

#include "TextArea.h"

#include "text/FontSet.h"
#include "GameData.h"
#include "ScrollBar.h"

using namespace std;



TextArea::TextArea()
{
	SetFont(FontSet::Get(14));
	SetColor(*GameData::Colors().Get("medium"));
}



TextArea::TextArea(const Rectangle &r)
	: TextArea()
{
	TextArea::SetRect(r);
}



// Stub destructor, so that unique_ptrs are destructed in the correct scope.
TextArea::~TextArea()
{
}



void TextArea::SetRect(const Rectangle &r)
{
	// TODO: Is there a use case where we would want the WrapWidth to be
	//       larger than the display width? We could handle this case by
	//       allowing the user to scroll left or right instead of up or
	//       down. This might be useful for overly long single lined text,
	//       or for vertical text layout.
	wrappedText.SetWrapWidth(r.Width());
	ScrollArea::SetRect(r);
}



void TextArea::SetText(const string &s)
{
	text = s;
	Invalidate();
}



void TextArea::SetFont(const Font &f)
{
	wrappedText.SetFont(f);
	Invalidate();
}



void TextArea::SetParagraphBreak(int height)
{
	wrappedText.SetParagraphBreak(height);
	Invalidate();
}



void TextArea::SetColor(const Color &c)
{
	color = c;
	Invalidate();
}



void TextArea::SetAlignment(Alignment a)
{
	wrappedText.SetAlignment(a);
	Invalidate();
}



void TextArea::SetTruncate(Truncate t)
{
	wrappedText.SetTruncate(t);
	Invalidate();
}



int TextArea::GetTextHeight(bool trailingBreak)
{
	Validate(trailingBreak);
	return wrappedText.Height(trailingBreak);
}



int TextArea::GetLongestLineWidth()
{
	Validate(scrollHeightIncludesTrailingBreak);
	return wrappedText.LongestLineWidth();
}



void TextArea::Validate(bool trailingBreak)
{
	if(!contentsIsValid || trailingBreak != scrollHeightIncludesTrailingBreak)
	{
		wrappedText.Wrap(text);
		scroll.SetMaxValue(wrappedText.Height(trailingBreak));
		scrollHeightIncludesTrailingBreak = trailingBreak;
		contentsIsValid = true;
	}
}



void TextArea::DrawText(const Point &topLeft)
{
	wrappedText.Draw(topLeft, color);
}
