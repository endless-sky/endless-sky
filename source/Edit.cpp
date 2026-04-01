/* Edit.cpp
Copyright (c) 2026 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Edit.h"

#include "Color.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Rectangle.h"
#include "Screen.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "UI.h"

#include <cassert>

#include <SDL2/SDL.h>

namespace
{
	constexpr size_t INVALID_POS = -1;
}



Edit::Edit()
{
	Clear();
	caret.SetHeight(fontSize);
}



void Edit::SetFontSize(int f)
{
	fontSize = f;
	caret.SetHeight(fontSize);
	ComputeTextBounds();
}



void Edit::SetPosition(const Rectangle& p)
{
	position = p;
	ComputeTextBounds();
}



const std::string& Edit::Text() const
{
	assert(historyPos > 0);
	return textHistory[historyPos - 1].first;
}



void Edit::SetText(const std::string& s)
{
	Clear();
	UpdateText(s, s.size());
}



void Edit::Clear()
{
	historyPos = 0;
	textHistory.clear();
	UpdateText("", 0); // Make sure the history is not empty.
	caretPos = 0;
	highlightPos = INVALID_POS;
}



void Edit::Draw()
{
	if(!visible)
		return;

	// const Font &font = FontSet::Get(14);
	// const Color &bright = *GameData::Colors().Get("bright");
	// const Color &dim = *GameData::Colors().Get("medium");
	if(bgColor == Color())
	{
		bgColor = *GameData::Colors().Get("panel background");
	}
	const Font &font = FontSet::Get(fontSize);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");

	FillShader::Fill(position.Center(), position.Dimensions(), bgColor);

	if(HasFocus())
	{
		caret.Draw();

		if(highlightPos != INVALID_POS)
		{
			Color highlightColor = active.Transparent(.25);

			FillShader::Fill(highlight, highlightColor);
		}
	}

	font.Draw(Text(),
		AlignedOffset(0),
		isActive ? (isHover ? hover : active) : inactive);
}



void Edit::SetPadding(int p)
{
	padding = p;
	ComputeTextBounds();
}




bool Edit::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(!isEditable)
		return false;

	// Force the caret blink cycle to on after any keypress.
	caret.BlinkOn();

	bool shift = (mod & KMOD_SHIFT) != 0;
	bool ctrl = (mod & KMOD_CTRL) != 0;

	switch(key)
	{
	case SDLK_TAB:
		if(shift)
			FocusPrev();
		else
			FocusNext();
		break;
	case SDLK_HOME:
		MoveCaret(0);
		break;
	case SDLK_END:
		MoveCaret(Text().size());
		break;
	case SDLK_LEFT:
		if(caretPos > 0)
			MoveCaret(caretPos - 1);
		break;
	case SDLK_RIGHT:
		if(caretPos < Text().size())
			MoveCaret(caretPos + 1);
		break;
	case SDLK_BACKSPACE:
		if(highlightPos != INVALID_POS)
		{
			int left = highlightPos;
			int right = caretPos;
			if(left > right)
				std::swap(left, right);
			UpdateText(Text().substr(0, left) + Text().substr(right), left);
			highlightPos = INVALID_POS;
		}
		else if(caretPos > 0)
		{
			UpdateText(Text().substr(0, caretPos - 1) + Text().substr(caretPos), caretPos - 1);
		}
		break;
	case SDLK_DELETE:
		if(highlightPos != INVALID_POS)
		{
			int left = highlightPos;
			int right = caretPos;
			if(left > right)
				std::swap(left, right);
			if(shift)
			{
				// shift-del, treat as cut
				SDL_SetClipboardText(Text().substr(left, right - left).c_str());
			}
			UpdateText(Text().substr(0, left) + Text().substr(right), left);
			highlightPos = INVALID_POS;
		}
		else if(caretPos < Text().size())
		{
			UpdateText(Text().substr(0, caretPos) + Text().substr(caretPos + 1), caretPos);
		}
		break;
	case SDLK_c:
		if(ctrl)
		{
			// ctrl-c copy
			Copy();
			break;
		}
		return false;
		break;
	case SDLK_x:
		if(ctrl)
		{
			// ctrl-x cut
			Cut();
			break;
		}
		return false;
		break;
	case SDLK_v:
		if(ctrl)
		{
			// ctrl-v paste
			Paste();
			break;
		}
		return false;
		break;
	case SDLK_z:
		if(ctrl)
		{
			if(shift)
			{
					// ctrl-shift-z Redo
					Redo();
			}
			else
			{
					// ctrl-z undo
					Undo();
			}
			break;
		}
		return false;
		break;
	case SDLK_INSERT:
		if(shift)
		{
			// shift-insert, paste
			Paste();
			break;
		}
		else if(ctrl)
		{
			// ctrl-insert, copy
			Copy();
			break;
		}
		return false;
		break;
	default:
		return false;
	}

	return true;
}



bool Edit::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;
	if(!position.Contains(Point(x, y)))
	{
		SetFocus(false);
		return false;
	}

	dragPos = Point(x, y);

	SetFocus(true);

	const std::string &s = Text();
	size_t newpos = 0;
	for(; newpos < s.size(); ++newpos)
	{
		if(AlignedOffset(newpos + 1).X() > x)
			break;
	}

	MoveCaret(newpos);

	return true;
}



bool Edit::Drag(double dx, double dy)
{
	if(dragPos != Point())
	{
		if(highlightPos == INVALID_POS)
			highlightPos = caretPos;
		dragPos.X() += dx;
		dragPos.Y() += dy;

		const std::string &s = Text();
		size_t newpos = 0;
		for(; newpos < s.size(); ++newpos)
		{
			if(AlignedOffset(newpos + 1).X() > dragPos.X())
				break;
		}

		UpdateHighlightRect(newpos, highlightPos);

		UpdateCaret(newpos);
		return true;
	}
	return false;
}



bool Edit::Release(int x, int y, MouseButton button)
{
	dragPos = Point();
	return false;
}



bool Edit::OnFocus(bool f)
{
	if(f)
	{
		if(!isEditable)
			return false;
		SDL_StartTextInput();
		highlightPos = INVALID_POS;
		UpdateCaret(textHistory.back().second);
	}
	else
		SDL_StopTextInput();
	return true;
}



bool Edit::TextInput(const std::string& s)
{
	if(!HasFocus())
		return false;

	std::string newText;
	size_t newPos = INVALID_POS;
	if(highlightPos != INVALID_POS)
	{
		int left = highlightPos;
		int right = caretPos;
		if(left > right)
			std::swap(left, right);

		newText = Text().substr(0, left) + s + Text().substr(right);
		newPos = left + s.size();

		highlightPos = INVALID_POS;
	}
	else
	{
		newText = Text().substr(0, caretPos) + s + Text().substr(caretPos);
		newPos = caretPos + s.size();
	}
	// Someday, we should make the text scroll and clip if we type something too
	// long, but for now, just ignore input if it is too big to fit.
	auto &font = FontSet::Get(fontSize);
	if(font.Width(newText) > textBounds.Width())
		return true;

	UpdateText(newText, newPos);
	return true;
}



void Edit::MoveCaret(size_t pos)
{
	assert(pos >= 0 && pos <= Text().size());
	if(pos < 0)
		pos = 0;
	else if(pos > Text().size())
		pos = Text().size();

	auto mod = SDL_GetModState();
	bool shift = (mod & KMOD_SHIFT) != 0;

	if(highlightPos == INVALID_POS && shift)
		highlightPos = caretPos;
	else if(highlightPos != INVALID_POS && !shift)
		highlightPos = INVALID_POS;
	if(highlightPos != INVALID_POS)
		UpdateHighlightRect(pos, highlightPos);

	UpdateCaret(pos);
}



void Edit::UpdateCaret(size_t pos)
{
	caret.SetX(AlignedOffset(pos).X());
	caretPos = pos;
}



void Edit::UpdateText(const std::string& text, size_t caretPos)
{
	std::string mutableText = text;
	if(changedCallback)
	{
		if(changedCallback(mutableText))
		{
			// The callback accepted the change, but may have modified it.
			if(caretPos > text.size())
				caretPos = text.size();
		}
		else
		{
			// Callback rejected the change.
			caretPos = textHistory.back().second;
			return;
		}
	}


	textHistory.resize(historyPos++);
	textHistory.emplace_back(mutableText, caretPos);
	UpdateCaret(caretPos);
}



void Edit::Cut()
{
	if(highlightPos == INVALID_POS)
	{
		SDL_SetClipboardText(Text().c_str());
		UpdateText("", 0);
	}
	else
	{
		int left = highlightPos;
		int right = caretPos;
		if(left > right)
			std::swap(left, right);
		SDL_SetClipboardText(Text().substr(left, right - left).c_str());
		UpdateText(Text().substr(0, left) + Text().substr(right), left);
	}
	highlightPos = INVALID_POS;
}



void Edit::Copy()
{
	if(highlightPos == INVALID_POS)
		SDL_SetClipboardText(Text().c_str());
	else
	{
		int left = highlightPos;
		int right = caretPos;
		if(left > right)
			std::swap(left, right);
		SDL_SetClipboardText(Text().substr(left, right - left).c_str());
	}
}



void Edit::Paste()
{
	std::shared_ptr<const char> text(SDL_GetClipboardText(), SDL_free);
	if(highlightPos == INVALID_POS)
	{
		// insert text at caret position
		UpdateText(
			Text().substr(0, caretPos) + text.get() + Text().substr(caretPos),
			caretPos + strlen(text.get())
		);
	}
	else
	{
		// replace highlighted text
		int left = highlightPos;
		int right = caretPos;
		if(left > right)
			std::swap(left, right);
		UpdateText(
			Text().substr(0, left) + text.get() + Text().substr(right),
			left + strlen(text.get())
		);
	}

	highlightPos = INVALID_POS;
}



void Edit::Undo()
{
	// The first entry is the initial string set by the
	// constructor. Don't go past it.
	if(historyPos > 1)
	{
		--historyPos;
		UpdateCaret(textHistory[historyPos - 1].second);
	}
}



void Edit::Redo()
{
	if(historyPos < textHistory.size())
	{
		++historyPos;
		UpdateCaret(textHistory[historyPos - 1].second);
	}
}


Point Edit::AlignedOffset(size_t offset)
{
	const Font &font = FontSet::Get(fontSize);
	const std::string &s = Text();
	Point ret{};
	switch(alignment)
	{
	case Edit::LEFT:
		ret = Point(textBounds.Left(), textBounds.Center().Y() - font.Height() / 2.0);
		break;
	case Edit::RIGHT:
		ret = Point(textBounds.Right() - font.Width(s), textBounds.Center().Y() - font.Height() / 2.0);
		break;
	default: // CENTER
		ret = textBounds.Center() - Point(font.Width(s) / 2.0, font.Height() / 2.0);
		break;
	}

	if(offset)
		ret.X() += font.Width(s.substr(0, offset));
	return ret;
}



void Edit::UpdateHighlightRect(size_t o1, size_t o2)
{
	assert(o1 != INVALID_POS);
	assert(o2 != INVALID_POS);
	int highlightLeft = AlignedOffset(o1).X();
	int highlightRight = AlignedOffset(o2).X();
	if(highlightLeft > highlightRight)
		std::swap(highlightLeft, highlightRight);
	Point tl(highlightLeft, textBounds.Center().Y() - fontSize / 2.0);
	Point br = tl;
	br.X() = highlightRight;
	br.Y() += fontSize;
	highlight = Rectangle::WithCorners(tl, br);
}



void Edit::ComputeTextBounds()
{
	textBounds = Rectangle(position.Center(), position.Dimensions() - Point(padding * 2, padding * 2));
	caret.SetY(textBounds.Center().Y() - fontSize / 2.0);
}
