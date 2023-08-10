/*
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Dropdown.h"
#include "FillShader.h"
#include "Rectangle.h"
#include "Screen.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Color.h"
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_timer.h>

void Dropdown::SetSelected(const std::string& s)
{
	selected_string = s;
	selected_index = -1;
	for (size_t i = 0; i < options.size(); ++i)
	{
		if (options[i] == s)
		{
			selected_index = i;
			break;
		}
	}
}

void Dropdown::SetSelectedIndex(int idx)
{
	if (idx >= static_cast<int>(options.size()) || idx < 0)
	{
		selected_index = -1;
		selected_string.clear();
	}
	else
	{
		selected_index = idx;
		selected_string = options[idx];
	}
}

void Dropdown::SetOptions(const std::vector<std::string>& options)
{
	this->options = options;
	if (!selected_string.empty())
	{
		SetSelected(selected_string);
	}
	else if (!options.empty())
	{
		SetSelectedIndex(0);
	}
}

Point AlignText(Dropdown::ALIGN alignment, const Font& font, const Rectangle& pos, const std::string& s)
{
	switch(alignment)
	{
	case Dropdown::LEFT:  return Point(pos.Left(), pos.Center().Y() - font.Height() / 2);
	case Dropdown::RIGHT: return Point(pos.Right() - font.Width(s), pos.Center().Y() - font.Height() / 2);
	default:					 return pos.Center() - Point(font.Width(s) / 2, font.Height() / 2);
	}
}

void Dropdown::Draw()
{
	if (!enabled)
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
	const Color &dim = *GameData::Colors().Get("dim");

	FillShader::Fill(position.Center(), position.Dimensions(), bg_color);
	Rectangle text_bounds(position.Center(), position.Dimensions() - Point(padding*2, padding*2));
	font.Draw(selected_string,
		AlignText(alignment, font, text_bounds, selected_string),
		is_active ? (is_hover ? hover : active) : inactive);

	if (is_popped)
	{
		Point bg_size{position.Width(), position.Height() * options.size()};
		// if we are close to the bottom of the screen, draw the options above
		// instead of below;
		bool is_drawn_down = !(position.Bottom() + bg_size.Y() > Screen::Bottom());

		const Rectangle bg_rect = is_drawn_down
			? Rectangle::FromCorner({position.Left(), position.Bottom()}, bg_size)
			: Rectangle::FromCorner({position.Left(), position.Top() - bg_size.Y()}, bg_size);

		// Draw outline
		FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions() + Point(2, 2), dim);
		// Draw background
		FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions(), bg_color);
		// Draw a highlight
		if (highlight_index >= 0)
		{
			const Rectangle highlight_rect = Rectangle::FromCorner(
				bg_rect.TopLeft() + Point(0, position.Height() * highlight_index),
				position.Dimensions());
			FillShader::Fill(highlight_rect.Center(), highlight_rect.Dimensions(),
				*GameData::Colors().Get("shop side panel background"));
		}
		Rectangle opt_pos(bg_rect.TopLeft() + position.Dimensions() * .5, position.Dimensions() - Point(padding*2, padding*2));

		for (size_t i = 0; i < options.size(); ++i)
		{
			font.Draw(options[i],
				AlignText(alignment, font, opt_pos, options[i]),
				static_cast<int>(i) == selected_index ? active : inactive);
			opt_pos += Point(0, position.Height());
		}
	}
}

bool Dropdown::MouseDown(int x, int y)
{
	if (!enabled)
		return false;

	mouse_pos = Point(x, y);

	if (!is_popped)
	{
		if (position.Contains(Point(x, y)))
		{
			is_popped = true;
			click_stamp = SDL_GetTicks();
			return true;
		}
	}
	else
	{
		int idx = IdxFromPoint(x, y);
		if (idx >= 0)
		{
			SetSelectedIndex(idx);
			if (changed_callback)
				changed_callback(selected_index, selected_string);
		}

		is_popped = false;
		return true; // mark it as handled whether it was in our popup or not
	}
	return false;
}

bool Dropdown::MouseMove(int dx, int dy)
{
	if (!enabled)
		return false;

	mouse_pos += Point(dx, dy);

	if (is_popped)
	{
		highlight_index = IdxFromPoint(mouse_pos.X(), mouse_pos.Y());
	}

	return is_popped;
}

bool Dropdown::MouseUp(int x, int y)
{
	if (!enabled)
		return false;
	if (is_popped)
	{
		// short click, leave it up. Long click, see what they selected
		if (SDL_GetTicks() - click_stamp < 500)
		{
			// short click... leave the popup up
			return true;
		}
		else
		{
			int idx = IdxFromPoint(x, y);
			if (idx >= 0)
			{
				SetSelectedIndex(idx);
				if (changed_callback)
					changed_callback(selected_index, selected_string);
			}
			is_popped = false;
			return true;
		}
	}
	return false;
}

bool Dropdown::Hover(int x, int y)
{
	is_hover = false;
	if (!is_popped)
	{
		if (position.Contains(Point(x, y)))
		{
			is_hover = true;
			return true;
		}
	}
	else
	{
		highlight_index = IdxFromPoint(x, y);
		if (highlight_index >= 0)
			return true;
	}

	return false;
}

int Dropdown::IdxFromPoint(int x, int y)
{
	Point bg_size{position.Width(), position.Height() * options.size()};
	// if we are close to the bottom of the screen, they will be drawn down
	// instead of up
	bool is_drawn_down = !(position.Bottom() + bg_size.Y() > Screen::Bottom());

	const Rectangle opt_rect = is_drawn_down
		? Rectangle::FromCorner({position.Left(), position.Bottom()}, bg_size)
		: Rectangle::FromCorner({position.Left(), position.Top() - bg_size.Y()}, bg_size);
	if (opt_rect.Contains(Point(x, y)))
	{
		int idx = static_cast<int>(y - opt_rect.Top()) / position.Height();
		// We have validated that we are within the dropdown, but floating point
		// errors still mean idx is occasionally out of bounds. Clamp it.
		if (idx < 0) idx = 0;
		if (idx >= static_cast<int>(options.size())) idx = options.size() - 1;
		return idx;
	}
	return -1;
}
