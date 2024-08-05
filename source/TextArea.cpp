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
#include "shader/PointerShader.h"
#include "Preferences.h"
#include "RenderBuffer.h"



TextArea::TextArea()
{
	SetInterruptible(true);
	SetTrapAllEvents(false);
	SetIsFullScreen(false);

	SetFont(FontSet::Get(14));
	SetColor(*GameData::Colors().Get("medium"));
}



TextArea::TextArea(const Rectangle &r): TextArea()
{
	SetRect(r);
}



// Stub destructor, so that unique_ptrs are destructed in the correct scope.
TextArea::~TextArea()
{
}



void TextArea::SetText(const std::string &s)
{
	text = s;
	Invalidate();
}



void TextArea::SetRect(const Rectangle &r)
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
	Invalidate();
}



void TextArea::SetFont(const Font &f)
{
	wrappedText.SetFont(f);
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



int TextArea::GetTextHeight()
{
	Validate();
	return wrappedText.Height();
}



int TextArea::GetLongestLineWidth()
{
	Validate();
	return wrappedText.LongestLineWidth();
}



void TextArea::Draw()
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
	const float POINTER_OFFSET = 5;
	if(scroll.Scrollable())
	{
		// Draw up and down pointers, mostly to indicate when scrolling
		// is possible, but might as well make them clickable too.
		Rectangle topRight(position + Point{buffer->Right(), buffer->Top() + POINTER_OFFSET}, {20.0, 20.0});
		PointerShader::Draw(topRight.Center(), UP,
			10.f, 10.f, 5.f, Color(scroll.IsScrollAtMin() ? .2f : .8f, 0.f));
		AddZone(topRight, [&]() { scroll.Scroll(-Preferences::ScrollSpeed()); });

		Rectangle bottomRight(position + Point{buffer->Right(), buffer->Bottom() - POINTER_OFFSET}, {20.0, 20.0});
		PointerShader::Draw(bottomRight.Center(), DOWN,
			10.f, 10.f, 5.f, Color(scroll.IsScrollAtMax() ? .2f : .8f, 0.f));
		AddZone(bottomRight, [&]() { scroll.Scroll(Preferences::ScrollSpeed()); });
	}
}



bool TextArea::Click(int x, int y, int clicks)
{
	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	dragging = bounds.Contains(Point(x, y));
	return dragging;
}



bool TextArea::Drag(double dx, double dy)
{
	if(dragging)
	{
		scroll.Scroll(-dy, 0);
		bufferIsValid = false;
		return true;
	}
	return false;
}



bool TextArea::Release(int x, int y)
{
	bool ret = dragging;
	dragging = false;
	return ret;
}



bool TextArea::Hover(int x, int y)
{
	if(!buffer)
		return false;
	Rectangle bounds(position, {buffer->Width(), buffer->Height()});
	hovering = bounds.Contains(Point(x, y));
	return hovering;
}



bool TextArea::Scroll(double dx, double dy)
{
	if(hovering)
	{
		scroll.Scroll(-dy * Preferences::ScrollSpeed());
		bufferIsValid = false;
	}
	return hovering;
}



void TextArea::Invalidate()
{
	bufferIsValid = false;
	textIsValid = false;
}



void TextArea::Validate()
{
	if(!textIsValid)
	{
		wrappedText.Wrap(text);
		scroll.SetMaxValue(wrappedText.Height());
		textIsValid = true;
	}
}
