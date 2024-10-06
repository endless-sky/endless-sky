/* TextAreaPanel.cpp
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

#include "TextAreaPanel.h"

#include "../../text/FontSet.h"
#include "../../GameData.h"
#include "../../PointerShader.h"
#include "../../Preferences.h"
#include "../../RenderBuffer.h"
#include "../../ScrollBar.h"



TextAreaPanel::TextAreaPanel()
{
	SetInterruptible(true);
	SetTrapAllEvents(false);
	SetIsFullScreen(false);

	SetFont(FontSet::Get(14));
	SetColor(*GameData::Colors().Get("medium"));
}



TextAreaPanel::TextAreaPanel(const Rectangle &r): TextAreaPanel()
{
	SetRect(r);
}



// Stub destructor, so that unique_ptrs are destructed in the correct scope.
TextAreaPanel::~TextAreaPanel()
{
}



void TextAreaPanel::SetText(const std::string &s)
{
	text = s;
	Invalidate();
}



void TextAreaPanel::SetRect(const Rectangle &r)
{
	// TODO: Is there a use case where we would want the WrapWidth to be
	//       larger than the display width? We could handle this case by
	//       allowing the user to scroll left or right instead of up or
	//       down. This might be useful for overly long single lined text,
	//       or for vertical text layout.
	position = r.Center();
	size = r.Dimensions();
	buffer.reset();
	wrappedText.SetWrapWidth(r.Width());
	scroll.SetDisplaySize(r.Height());
	scrollBar.displaySizeFraction = scroll.DisplaySize() / scroll.MaxValue();
	Invalidate();
}



void TextAreaPanel::SetFont(const Font &f)
{
	wrappedText.SetFont(f);
	Invalidate();
}



void TextAreaPanel::SetColor(const Color &c)
{
	color = c;
	Invalidate();
}



void TextAreaPanel::SetAlignment(Alignment a)
{
	wrappedText.SetAlignment(a);
	Invalidate();
}



void TextAreaPanel::SetTruncate(Truncate t)
{
	wrappedText.SetTruncate(t);
	Invalidate();
}



int TextAreaPanel::GetTextHeight()
{
	Validate();
	return wrappedText.Height();
}



int TextAreaPanel::GetLongestLineWidth()
{
	Validate();
	return wrappedText.LongestLineWidth();
}



void TextAreaPanel::Draw()
{
	if(!buffer)
		buffer = std::make_unique<RenderBuffer>(size);

	Validate();
	if(!bufferIsValid || !scroll.IsAnimationDone())
	{
		scroll.Step();

		auto target = buffer->SetTarget();
		Point topLeft(buffer->Left(), buffer->Top() - scroll.AnimatedValue());
		wrappedText.Draw(topLeft, color);
		target.Deactivate();

		buffer->SetFadePadding(
			scroll.IsScrollAtMin() ? 0 : 20,
			scroll.IsScrollAtMax() ? 0 : 20
		);
		bufferIsValid = true;
	}
	buffer->Draw(position);

	const Point UP{0, -1};
	const Point DOWN{0, 1};
	const float SCROLLBAR_OFFSET = 5;
	const float POINTER_OFFSET = 5;
	if(scroll.Scrollable())
	{
		Point topRight(position + Point(buffer->Right() + SCROLLBAR_OFFSET, buffer->Top() + POINTER_OFFSET));
		Point bottomRight(position + Point(buffer->Right() + SCROLLBAR_OFFSET, buffer->Bottom() - POINTER_OFFSET));

		scrollBar.SyncDraw(scroll, topRight, bottomRight);
	}
}



bool TextAreaPanel::Click(int x, int y, int clicks)
{
	if(scrollBar.SyncClick(scroll, x, y, clicks))
	{
		bufferIsValid = false;
		return true;
	}

	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	dragging = bounds.Contains(Point(x, y));
	return dragging;
}



bool TextAreaPanel::Drag(double dx, double dy)
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



bool TextAreaPanel::Release(int x, int y)
{
	bool ret = dragging;
	dragging = false;
	return ret;
}



bool TextAreaPanel::Hover(int x, int y)
{
	scrollBar.Hover(x, y);

	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	hovering = bounds.Contains(Point(x, y));
	return hovering;
}



bool TextAreaPanel::Scroll(double dx, double dy)
{
	if(hovering)
	{
		scroll.Scroll(-dy * Preferences::ScrollSpeed());
		bufferIsValid = false;
	}
	return hovering;
}



void TextAreaPanel::Invalidate()
{
	bufferIsValid = false;
	textIsValid = false;
}



void TextAreaPanel::Validate()
{
	if(!textIsValid)
	{
		wrappedText.Wrap(text);
		scroll.SetMaxValue(wrappedText.Height());
		textIsValid = true;
	}
}
