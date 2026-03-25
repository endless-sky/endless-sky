/* Dropdown.cpp
Copyright (c) 2023 by thewierdnut

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

#include <SDL2/SDL.h>


class Dropdown::DroppedPanel: public Panel
{
public:
	DroppedPanel(Dropdown *parent);
	virtual ~DroppedPanel() = default;

	void SetMousePos(const Point &p) { mouse_pos = p; }

protected:
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Release(int x, int y, MouseButton button) override;
	virtual bool Hover(int x, int y) override;

	virtual void Draw() override;

private:
	Dropdown *dd = nullptr;

	uint32_t click_stamp = 0;
	Point mouse_pos;

	int highlight_index = -1;
};



Dropdown::Dropdown()
{
	SetTypeable(false);
}



void Dropdown::SetText(const std::string &s)
{
	Edit::SetText(s);
	selected_index = -1;
	for(size_t i = 0; i < options.size(); ++i)
	{
		if(options[i] == s)
		{
			selected_index = i;
			break;
		}
	}
}



void Dropdown::SetSelectedIndex(int idx)
{
	if(idx >= static_cast<int>(options.size()) || idx < 0)
	{
		selected_index = -1;
		Clear();
	}
	else
	{
		selected_index = idx;
		SetText(options[idx]);
	}
}



void Dropdown::SetOptions(const std::vector<std::string> &options)
{
	this->options = options;
	if(!Text().empty())
	{
		SetText(Text());
	}
	else if(!options.empty())
	{
		SetSelectedIndex(0);
	}
}



Point AlignText(Dropdown::ALIGN alignment, const Font &font, const Rectangle &pos, const std::string &s)
{
	switch(alignment)
	{
	case Dropdown::LEFT:
		return Point(pos.Left(), pos.Center().Y() - font.Height() / 2.0);
	case Dropdown::RIGHT:
		return Point(pos.Right() - font.Width(s), pos.Center().Y() - font.Height() / 2.0);
	default:
		return pos.Center() - Point(font.Width(s) / 2.0, font.Height() / 2.0);
	}
}



void Dropdown::Draw()
{
	Edit::Draw();
	ClearZones();
	if(!Visible())
		return;

	if(showDropIcon || Edit::Enabled())
	{
		Point dropIconPos = Position().Center();
		dropIconPos.X() += Position().Width() / 2 - Position().Height() / 2;
		SpriteShader::Draw(SpriteSet::Get("ui/sort descending"), dropIconPos, FontSize() / 24.0);
	}

	if(Enabled())
	{
		if(Edit::Enabled())
		{
			// Only use the drop icon as the click zone, so that the user can
			// still click and highlight text.
			Point dropIconPos = Position().Center();
			dropIconPos.X() += Position().Width() / 2 - Position().Height() / 2;
			Rectangle pz(dropIconPos, Point(Position().Height(), Position().Height()));
			AddZone(pz, [this](const Panel::Event &e) {
				DoDropdown(e.pos);
			});
		}
		else
		{
			// Use the whole control.
			AddZone(Position(), [this](const Panel::Event &e) {
				DoDropdown(e.pos);
			});
		}

	}
}



void Dropdown::SetTypeable(bool t)
{
	Edit::SetEnabled(t);
}



void Dropdown::SetEnabled(bool e)
{
	// This defintion is intentionally masking the definition in the base class.
	enabled = e;
}



void Dropdown::DoDropdown(const Point &pos)
{
	auto p = std::make_shared<DroppedPanel>(this);
	GetUI().Push(p);
	p->SetMousePos(pos);
}



int Dropdown::IdxFromPoint(int x, int y) const
{
	Point bg_size{Position().Width(), Position().Height() * options.size()};
	// If the Dropdown is too close to the bottom, then display the DroppedPanel
	// above the Dropdown instead of below it.
	bool is_drawn_down = !(Position().Bottom() + bg_size.Y() > Screen::Bottom());

	const Rectangle opt_rect = is_drawn_down
		? Rectangle::FromCorner({Position().Left(), Position().Bottom()}, bg_size)
		: Rectangle::FromCorner({Position().Left(), Position().Top() - bg_size.Y()}, bg_size);
	if(opt_rect.Contains(Point(x, y)))
	{
		int idx = static_cast<int>(y - opt_rect.Top()) / Position().Height();
		// Clamp the idx so that floating point errors don't force it out of
		// bounds.
		if(idx < 0) idx = 0;
		if(idx >= static_cast<int>(options.size())) idx = options.size() - 1;
		return idx;
	}
	return -1;
}



Dropdown::DroppedPanel::DroppedPanel(Dropdown *parent):
	dd(parent),
	click_stamp(SDL_GetTicks())
{
	SetTrapAllEvents(true);
	SetInterruptible(false);
}



bool Dropdown::DroppedPanel::Click(int x, int y, MouseButton, int clicks)
{
	mouse_pos = Point(x, y);

	int idx = dd->IdxFromPoint(x, y);
	if(idx >= 0)
		dd->SetSelectedIndex(idx);

	GetUI().Pop(this);
	// this pointer no longer safe to access.

	return true;
}



bool Dropdown::DroppedPanel::Drag(double dx, double dy)
{
	mouse_pos += Point(dx, dy);
	highlight_index = dd->IdxFromPoint(mouse_pos.X(), mouse_pos.Y());

	return true;
}



bool Dropdown::DroppedPanel::Release(int x, int y, MouseButton)
{
	// short click, leave it up. Long click, see what they selected
	if(SDL_GetTicks() - click_stamp < 500)
	{
		// short click... leave the popup up
	}
	else
	{
		// long click and drag.
		int idx = dd->IdxFromPoint(x, y);
		if(idx >= 0)
		{
			dd->SetSelectedIndex(idx);
		}
		GetUI().Pop(this);
		// this pointer no longer safe to access.
	}
	return true;
}



bool Dropdown::DroppedPanel::Hover(int x, int y)
{
	highlight_index = dd->IdxFromPoint(x, y);
	return highlight_index >= 0;
}



void Dropdown::DroppedPanel::Draw()
{
	const Font &font = FontSet::Get(dd->FontSize());
	const Color &active = *GameData::Colors().Get("active");
	const Color &inactive = *GameData::Colors().Get("inactive");
	const Color &dim = *GameData::Colors().Get("dim");

	Point bg_size{dd->Position().Width(), dd->Position().Height() * dd->options.size()};
	// If the Dropdown is close to the bottom of the screen, then draw the
	// DroppedPanel above the Dropdown instead of below it.
	bool is_drawn_down = !(dd->Position().Bottom() + bg_size.Y() > Screen::Bottom());

	const Rectangle bg_rect = is_drawn_down
		? Rectangle::FromCorner({dd->Position().Left(), dd->Position().Bottom()}, bg_size)
		: Rectangle::FromCorner({dd->Position().Left(), dd->Position().Top() - bg_size.Y()}, bg_size);

	// Draw outline
	FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions() + Point(2, 2), dim);
	// Draw background
	FillShader::Fill(bg_rect.Center(), bg_rect.Dimensions(), dd->BgColor());
	// Draw a highlight
	if(highlight_index >= 0)
	{
		const Rectangle highlight_rect = Rectangle::FromCorner(
			bg_rect.TopLeft() + Point(0, dd->Position().Height() * highlight_index),
			dd->Position().Dimensions());
		FillShader::Fill(highlight_rect.Center(), highlight_rect.Dimensions(),
			*GameData::Colors().Get("shop side panel background"));
	}
	Rectangle opt_pos(
		bg_rect.TopLeft() + dd->Position().Dimensions() * .5,
		dd->Position().Dimensions() - Point(dd->Padding() * 2, dd->Padding() * 2)
	);

	for(size_t i = 0; i < dd->options.size(); ++i)
	{
		font.Draw(dd->options[i],
			AlignText(dd->Align(), font, opt_pos, dd->options[i]),
			static_cast<int>(i) == dd->selected_index ? active : inactive);
		opt_pos += Point(0, dd->Position().Height());
	}
}
