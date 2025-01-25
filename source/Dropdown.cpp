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
#include "shader/FillShader.h"
#include "Rectangle.h"
#include "Screen.h"
#include "image/SpriteSet.h"
#include "shader/SpriteShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Color.h"
#include "UI.h"

#include <SDL2/SDL.h>


class Dropdown::DroppedPanel: public Panel
{
public:
	DroppedPanel(Dropdown* parent);
	virtual ~DroppedPanel() = default;

	void SetMousePos(const Point& p) { mouse_pos = p; }

protected:
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y) override;
	virtual bool Hover(int x, int y) override;
	virtual bool ControllerButtonDown(SDL_GameControllerButton button) override;
	virtual bool ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive) override;

	virtual void Draw() override;

private:
	Dropdown* dd = nullptr;

	uint32_t click_stamp = 0;
	Point mouse_pos;

	int highlight_index = -1;
};



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
	case Dropdown::LEFT:  return Point(pos.Left(), pos.Center().Y() - font.Height() / 2.0);
	case Dropdown::RIGHT: return Point(pos.Right() - font.Width(s), pos.Center().Y() - font.Height() / 2.0);
	default:					 return pos.Center() - Point(font.Width(s) / 2.0, font.Height() / 2.0);
	}
}



void Dropdown::Draw()
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
	
	Rectangle text_bounds(position.Center(), position.Dimensions() - Point(padding*2, padding*2));
	font.Draw(selected_string,
		AlignText(alignment, font, text_bounds, selected_string),
		is_active ? (is_hover ? hover : active) : inactive);

	if(showDropIcon)
	{
		Point dropIconPos = position.Center();
		dropIconPos.X() += position.Width()/2 - position.Height()/2;
		SpriteShader::Draw(SpriteSet::Get("ui/sort descending"), dropIconPos);
	}

	if(enabled)
		AddZone(position, [this](const Panel::Event &e){ DoDropdown(e.pos); });
}



void Dropdown::DoDropdown(const Point &pos)
{
	auto p = std::make_shared<DroppedPanel>(this);
	GetUI()->Push(p);
	p->SetMousePos(pos);
}



int Dropdown::IdxFromPoint(int x, int y) const
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



Dropdown::DroppedPanel::DroppedPanel(Dropdown* parent):
	dd(parent),
	click_stamp(SDL_GetTicks())
{
	SetTrapAllEvents(true);
	SetInterruptible(false);
}



bool Dropdown::DroppedPanel::Click(int x, int y, int clicks)
{
	mouse_pos = Point(x, y);

	int idx = dd->IdxFromPoint(x, y);
	if (idx >= 0)
	{
		dd->SetSelectedIndex(idx);
		if (dd->changed_callback)
			dd->changed_callback(dd->selected_index, dd->selected_string);
	}

	GetUI()->Pop(this);
	// this pointer no longer safe to access.

	return true;
}



bool Dropdown::DroppedPanel::Drag(double dx, double dy)
{
	mouse_pos += Point(dx, dy);
	highlight_index = dd->IdxFromPoint(mouse_pos.X(), mouse_pos.Y());

	return true;
}



bool Dropdown::DroppedPanel::Release(int x, int y)
{
	// short click, leave it up. Long click, see what they selected
	if (SDL_GetTicks() - click_stamp < 500)
	{
		// short click... leave the popup up
	}
	else
	{
		// long click and drag.
		int idx = dd->IdxFromPoint(x, y);
		if (idx >= 0)
		{
			dd->SetSelectedIndex(idx);
			if (dd->changed_callback)
				dd->changed_callback(dd->selected_index, dd->selected_string);
		}
		GetUI()->Pop(this);
		// this pointer no longer safe to access.
	}
	return true;
}



bool Dropdown::DroppedPanel::Hover(int x, int y)
{
	highlight_index = dd->IdxFromPoint(x, y);
	return highlight_index >= 0;
}



bool Dropdown::DroppedPanel::ControllerButtonDown(SDL_GameControllerButton button)
{
	if(button == SDL_CONTROLLER_BUTTON_A)
	{
		if(highlight_index >= 0 && highlight_index < static_cast<int>(dd->options.size()))
		{
			dd->SetSelectedIndex(highlight_index);
			if (dd->changed_callback)
				dd->changed_callback(dd->selected_index, dd->selected_string);
		}
	}
	GetUI()->Pop(this);
	return true;
}



bool Dropdown::DroppedPanel::ControllerTriggerPressed(SDL_GameControllerAxis axis, bool positive)
{
	// Don't really care what axis is used, just its direction
	if(positive)
	{
		if(highlight_index < 0 || highlight_index >= static_cast<int>(dd->options.size()))
			highlight_index = 0;
		else
		{
			++highlight_index;
			if(highlight_index >= static_cast<int>(dd->options.size()))
				highlight_index = dd->options.size() - 1;
		}
	}
	else
	{
		if(highlight_index < 0 || highlight_index >= static_cast<int>(dd->options.size()))
			highlight_index = dd->options.size() - 1;
		else
		{
			--highlight_index;
			if(highlight_index < 0)
				highlight_index = 0;
		}
	}

	return true;
}



void Dropdown::DroppedPanel::Draw()
{
	const Font &font = FontSet::Get(dd->font_size);
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");
	const Color &dim = *GameData::Colors().Get("dim");

	Point bg_size{dd->position.Width(), dd->position.Height() * dd->options.size()};
	// if we are close to the bottom of the screen, draw the options above
	// instead of below;
	bool is_drawn_down = !(dd->position.Bottom() + bg_size.Y() > Screen::Bottom());

	const Rectangle bg_rect = is_drawn_down
		? Rectangle::FromCorner({dd->position.Left(), dd->position.Bottom()}, bg_size)
		: Rectangle::FromCorner({dd->position.Left(), dd->position.Top() - bg_size.Y()}, bg_size);

	// Draw outline
	FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions() + Point(2, 2), dim);
	// Draw background
	FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions(), dd->bg_color);
	// Draw a highlight
	if (highlight_index >= 0)
	{
		const Rectangle highlight_rect = Rectangle::FromCorner(
			bg_rect.TopLeft() + Point(0, dd->position.Height() * highlight_index),
			dd->position.Dimensions());
		FillShader::Fill(highlight_rect.Center(), highlight_rect.Dimensions(),
			*GameData::Colors().Get("shop side panel background"));
	}
	Rectangle opt_pos(bg_rect.TopLeft() + dd->position.Dimensions() * .5, dd->position.Dimensions() - Point(dd->padding*2, dd->padding*2));

	for (size_t i = 0; i < dd->options.size(); ++i)
	{
		font.Draw(dd->options[i],
			AlignText(dd->alignment, font, opt_pos, dd->options[i]),
			static_cast<int>(i) == dd->selected_index ? active : inactive);
		opt_pos += Point(0, dd->position.Height());
	}
}