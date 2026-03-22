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
	caret.SetHeight(font_size);
}



void Edit::SetFontSize(int f)
{
	font_size = f;
	caret.SetHeight(font_size);
	ComputeTextBounds();
}



void Edit::SetPosition(const Rectangle& p)
{
	position = p;
	ComputeTextBounds();
}



const std::string& Edit::Text() const
{
	assert(history_pos > 0);
	return text_history[history_pos - 1].first;
}



void Edit::SetText(const std::string& s)
{
	Clear();
	UpdateText(s, s.size());
}



void Edit::Clear()
{
	history_pos = 0;
	text_history.clear();
	UpdateText("", 0); // Make sure the history is not empty.
	caret_pos = 0;
	highlight_pos = INVALID_POS;
}



void Edit::Draw()
{
	if(!visible)
		return;

	// const Font &font = FontSet::Get(14);
	// const Color &bright = *GameData::Colors().Get("bright");
	// const Color &dim = *GameData::Colors().Get("medium");
	if(bg_color == Color())
	{
		bg_color = *GameData::Colors().Get("panel background");
	}
	const Font &font = FontSet::Get(font_size);
	const Color &hover = *GameData::Colors().Get("hover");
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");

	FillShader::Fill(position.Center(), position.Dimensions(), bg_color);

	if(HasFocus())
	{
		caret.Draw();

		if(highlight_pos != INVALID_POS)
		{
			Color highlight_color = active.Transparent(.25);

			FillShader::Fill(highlight, highlight_color);
		}
	}

	font.Draw(Text(),
		AlignedOffset(0),
		is_active ? (is_hover ? hover : active) : inactive);
}



void Edit::SetPadding(int p)
{
	padding = p;
	ComputeTextBounds();
}




bool Edit::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(!is_editable)
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
		if(caret_pos > 0)
			MoveCaret(caret_pos - 1);
		break;
	case SDLK_RIGHT:
		if(caret_pos < Text().size())
			MoveCaret(caret_pos + 1);
		break;
	case SDLK_BACKSPACE:
		if(highlight_pos != INVALID_POS)
		{
			int left = highlight_pos;
			int right = caret_pos;
			if(left > right)
				std::swap(left, right);
			UpdateText(Text().substr(0, left) + Text().substr(right), left);
			highlight_pos = INVALID_POS;
		}
		else if(caret_pos > 0)
		{
			UpdateText(Text().substr(0, caret_pos - 1) + Text().substr(caret_pos), caret_pos - 1);
		}
		break;
	case SDLK_DELETE:
		if(highlight_pos != INVALID_POS)
		{
			int left = highlight_pos;
			int right = caret_pos;
			if(left > right)
				std::swap(left, right);
			if(shift)
			{
				// shift-del, treat as cut
				SDL_SetClipboardText(Text().substr(left, right - left).c_str());
			}
			UpdateText(Text().substr(0, left) + Text().substr(right), left);
			highlight_pos = INVALID_POS;
		}
		else if(caret_pos < Text().size())
		{
			UpdateText(Text().substr(0, caret_pos) + Text().substr(caret_pos + 1), caret_pos);
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

	drag_pos = Point(x, y);

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
	if(drag_pos != Point())
	{
		if(highlight_pos == INVALID_POS)
			highlight_pos = caret_pos;
		drag_pos.X() += dx;
		drag_pos.Y() += dy;

		const std::string &s = Text();
		size_t newpos = 0;
		for(; newpos < s.size(); ++newpos)
		{
			if(AlignedOffset(newpos + 1).X() > drag_pos.X())
				break;
		}

		UpdateHighlightRect(newpos, highlight_pos);

		UpdateCaret(newpos);
		return true;
	}
	return false;
}



bool Edit::Release(int x, int y, MouseButton button)
{
	drag_pos = Point();
	return false;
}



bool Edit::OnFocus(bool f)
{
	if(f)
	{
		if(!is_editable)
			return false;
		SDL_StartTextInput();
		highlight_pos = INVALID_POS;
		UpdateCaret(text_history.back().second);
	}
	else
		SDL_StopTextInput();
	return true;
}



bool Edit::TextInput(const std::string& s)
{
	if(!HasFocus())
		return false;

	std::string new_text;
	size_t new_pos = INVALID_POS;
	if(highlight_pos != INVALID_POS)
	{
		int left = highlight_pos;
		int right = caret_pos;
		if(left > right)
			std::swap(left, right);
		
		new_text = Text().substr(0, left) + s + Text().substr(right);
		new_pos = left + s.size();

		highlight_pos = INVALID_POS;
	}
	else
	{
		new_text = Text().substr(0, caret_pos) + s + Text().substr(caret_pos);
		new_pos = caret_pos + s.size();
	}
	// Someday, we should make the text scroll and clip if we type something too
	// long, but for now, just ignore input if it is too big to fit.
	auto &font = FontSet::Get(font_size);
	if(font.Width(new_text) > text_bounds.Width())
		return true;

	UpdateText(new_text, new_pos);
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
	
	if(highlight_pos == INVALID_POS && shift)
		highlight_pos = caret_pos;
	else if(highlight_pos != INVALID_POS && !shift)
		highlight_pos = INVALID_POS;
	if(highlight_pos != INVALID_POS)
		UpdateHighlightRect(pos, highlight_pos);

	UpdateCaret(pos);
}



void Edit::UpdateCaret(size_t pos)
{
	caret.SetX(AlignedOffset(pos).X());
	caret_pos = pos;
}



void Edit::UpdateText(const std::string& text, size_t caret_pos)
{
	std::string mutable_text = text;
	if(changed_callback)
	{
		if(changed_callback(mutable_text))
		{
			// The callback accepted the change, but may have modified it.
			if(caret_pos > text.size())
				caret_pos = text.size();
		}
		else
		{
			// Callback rejected the change.
			caret_pos = text_history.back().second;
			return;
		}
	}


	text_history.resize(history_pos++);
	text_history.emplace_back(mutable_text, caret_pos);
	UpdateCaret(caret_pos);
}



void Edit::Cut()
{
	if(highlight_pos == INVALID_POS)
	{
		SDL_SetClipboardText(Text().c_str());
		UpdateText("", 0);
	}
	else
	{
		int left = highlight_pos;
		int right = caret_pos;
		if(left > right)
			std::swap(left, right);
		SDL_SetClipboardText(Text().substr(left, right - left).c_str());
		UpdateText(Text().substr(0, left) + Text().substr(right), left);
	}
	highlight_pos = INVALID_POS;
}



void Edit::Copy()
{
	if(highlight_pos == INVALID_POS)
		SDL_SetClipboardText(Text().c_str());
	else
	{
		int left = highlight_pos;
		int right = caret_pos;
		if(left > right)
			std::swap(left, right);
		SDL_SetClipboardText(Text().substr(left, right - left).c_str());
	}
}



void Edit::Paste()
{
	std::shared_ptr<const char> text(SDL_GetClipboardText(), SDL_free);
	if(highlight_pos == INVALID_POS)
	{
		// insert text at caret position
		UpdateText(
			Text().substr(0, caret_pos) + text.get() + Text().substr(caret_pos),
			caret_pos + strlen(text.get())
		);
	}
	else
	{
		// replace highlighted text
		int left = highlight_pos;
		int right = caret_pos;
		if(left > right)
			std::swap(left, right);
		UpdateText(
			Text().substr(0, left) + text.get() + Text().substr(right),
			left + strlen(text.get())
		);
	}

	highlight_pos = INVALID_POS;
}



void Edit::Undo()
{
	// The first entry is the initial string set by the
	// constructor. Don't go past it.
	if(history_pos > 1)
	{
		--history_pos;
		UpdateCaret(text_history[history_pos - 1].second);
	}
}



void Edit::Redo()
{
	if(history_pos < text_history.size())
	{
		++history_pos;
		UpdateCaret(text_history[history_pos - 1].second);
	}
}


Point Edit::AlignedOffset(size_t offset)
{
	const Font &font = FontSet::Get(font_size);
	const std::string &s = Text();
	Point ret{};
	switch(alignment)
	{
	case Edit::LEFT:
		ret = Point(text_bounds.Left(), text_bounds.Center().Y() - font.Height() / 2.0);
		break;
	case Edit::RIGHT:
		ret = Point(text_bounds.Right() - font.Width(s), text_bounds.Center().Y() - font.Height() / 2.0);
		break;
	default: // CENTER
		ret = text_bounds.Center() - Point(font.Width(s) / 2.0, font.Height() / 2.0);
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
	int highlight_left = AlignedOffset(o1).X();
	int highlight_right = AlignedOffset(o2).X();
	if(highlight_left > highlight_right)
		std::swap(highlight_left, highlight_right);
	Point tl(highlight_left, text_bounds.Center().Y() - font_size / 2.0);
	Point br = tl;
	br.X() = highlight_right;
	br.Y() += font_size;
	highlight = Rectangle::WithCorners(tl, br);
}



void Edit::ComputeTextBounds()
{
	text_bounds = Rectangle(position.Center(), position.Dimensions() - Point(padding * 2, padding * 2));
	caret.SetY(text_bounds.Center().Y() - font_size / 2.0);
}
