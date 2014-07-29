/* InfoPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "InfoPanel.h"

#include "Armament.h"
#include "Color.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "MissionPanel.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "Ship.h"
#include "ShipInfoDisplay.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <cctype>

using namespace std;



InfoPanel::InfoPanel(PlayerInfo &player)
	: player(player), shipIt(player.Ships().begin())
{
	UpdateInfo();
}



void InfoPanel::Draw() const
{
	// Darken everything but the dialog.
	Color back(0., .7);
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
	
	Information interfaceInfo;
	if(player.Ships().size() > 1)
		interfaceInfo.SetCondition("has other ships");
	const Interface *interface = GameData::Interfaces().Get("ship info");
	interface->Draw(interfaceInfo);
	
	if(player.Ships().empty())
		return;
	
	Color dim(.5, 0.);
	Color bright(.8, 0.);
	const Font &font = FontSet::Get(14);
	const Ship &ship = **shipIt;
	
	// Left column: basic ship attributes.
	font.Draw("ship:", Point(-490., -270.), dim);
	Point shipNamePos(-260 - font.Width(ship.Name()), -270.);
	font.Draw(ship.Name(), shipNamePos, bright);
	font.Draw("model:", Point(-490., -250.), dim);
	Point modelNamePos(-260 - font.Width(ship.ModelName()), -250.);
	font.Draw(ship.ModelName(), modelNamePos, bright);
	info.DrawAttributes(Point(-500., -240.));
	
	// Outfits list.
	Point pos(-240., -270.);
	for(const auto &it : outfits)
	{
		int height = 20 * (it.second.size() + 1);
		if(pos.X() == -240. && pos.Y() + height > 20.)
			pos = Point(pos.X() + 250., -270.);
		
		font.Draw(it.first, pos, bright);
		for(const Outfit *outfit : it.second)
		{
			pos.Y() += 20.;
			font.Draw(outfit->Name(), pos, dim);
			string number = to_string(ship.OutfitCount(outfit));
			Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
			font.Draw(number, numberPos, bright);
		}
		pos.Y() += 30.;
	}
	
	// Cargo list.
	const CargoHold &cargo = (player.Cargo().Used() ? player.Cargo() : ship.Cargo());
	pos = Point(260., -270.);
	if(cargo.CommoditiesSize() || cargo.HasOutfits() || cargo.MissionCargoSize())
	{
		font.Draw("Cargo", pos, bright);
		pos.Y() += 20.;
	}
	if(cargo.CommoditiesSize())
	{
		for(const auto &it : cargo.Commodities())
		{
			string number = to_string(it.second);
			Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
			font.Draw(it.first, pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += 20.;
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 250.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.HasOutfits())
	{
		for(const auto &it : cargo.Outfits())
		{
			string number = to_string(it.second);
			Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
			font.Draw(it.first->Name(), pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += 20.;
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 250.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.MissionCargoSize())
	{
		for(const auto &it : cargo.MissionCargo())
		{
			// Capitalize the name of the cargo.
			string name = it.first->Cargo();
			bool first = true;
			for(char &c : name)
			{
				if(isspace(c))
					first = true;
				else
				{
					if(first && islower(c))
						c = toupper(c);
					first = false;
				}
			}
			
			string number = to_string(it.second);
			Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
			font.Draw(name, pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += 20.;
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 250.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.Passengers())
	{
		pos = Point(pos.X(), 250.);
		string number = to_string(cargo.Passengers());
		Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
		font.Draw("passengers:", pos, dim);
		font.Draw(number, numberPos, bright);
	}
	
	// Weapon positions.
	const Sprite *sprite = ship.GetSprite().GetSprite();
	double scale = min(240. / sprite->Width(), 240. / sprite->Height());
	Point shipCenter(-125., 145.);
	SpriteShader::Draw(sprite, shipCenter, scale, 7);
	
	Color black(0., 1.);
	pos = Point(10., 250.);
	for(unsigned i = 0; i < ship.Weapons().size(); ++i)
	{
		const Armament::Weapon &weapon = ship.Weapons()[i];
		if(weapon.IsTurret())
		{
			DrawWeapon(i, pos, shipCenter + (2. * scale) * weapon.GetPoint());
			pos.Y() -= 20.;
		}
	}
	if(pos.Y() != 250.)
		pos.Y() -= 10.;
	for(unsigned i = 0; i < ship.Weapons().size(); ++i)
	{
		const Armament::Weapon &weapon = ship.Weapons()[i];
		if(!weapon.IsTurret())
		{
			DrawWeapon(i, pos, shipCenter + (2. * scale) * weapon.GetPoint());
			pos.Y() -= 20.;
		}
	}
	
	// Re-positioning weapons.
	if(selectedWeapon >= 0)
	{
		const string &name = ship.Weapons()[selectedWeapon].GetOutfit()->Name();
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



bool InfoPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == 'd')
		GetUI()->Pop(this);
	else if(!player.Ships().empty() && (key == 'p' || key == SDLK_LEFT || key == SDLK_UP))
	{
		if(shipIt == player.Ships().begin())
			shipIt = player.Ships().end();
		--shipIt;
		UpdateInfo();
	}
	else if(!player.Ships().empty() && (key == 'n' || key == SDLK_RIGHT || key == SDLK_DOWN))
	{
		++shipIt;
		if(shipIt == player.Ships().end())
			shipIt = player.Ships().begin();
		UpdateInfo();
	}
	else if(key == 'm')
		GetUI()->Push(new MissionPanel(player));
	
	return true;
}



bool InfoPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("ship info");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key != '\0')
			return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	if(shipIt == player.Ships().end())
		return true;
	
	if(hoverWeapon >= 0 && (**shipIt).GetPlanet())
	{
		if(selectedWeapon == -1)
			selectedWeapon = hoverWeapon;
		else
		{
			(**shipIt).GetArmament().Swap(hoverWeapon, selectedWeapon);
			selectedWeapon = -1;
		}
	}
	else if(selectedWeapon)
		selectedWeapon = -1;
	
	return true;
}



bool InfoPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	
	const vector<Armament::Weapon> &weapons = (**shipIt).Weapons();
	hoverWeapon = -1;
	for(const ClickZone &zone : zones)
		if(zone.Contains(x, y))
		{
			if(selectedWeapon == -1)
				hoverWeapon = zone.Index();
			if(weapons[selectedWeapon].IsTurret() == weapons[zone.Index()].IsTurret())
				hoverWeapon = zone.Index();
		}
	
	return true;
}



void InfoPanel::UpdateInfo()
{
	selectedWeapon = -1;
	hoverWeapon = -1;
	if(shipIt == player.Ships().end())
		return;
	
	const Ship &ship = **shipIt;
	info.Update(ship);
	
	outfits.clear();
	for(const auto &it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);
}



void InfoPanel::DrawWeapon(int index, const Point &pos, const Point &hardpoint) const
{
	const Font &font = FontSet::Get(14);
	double high = (index == hoverWeapon ? .8 : .5);
	Color textColor(high, 0.);
	font.Draw((**shipIt).Weapons()[index].GetOutfit()->Name(), pos, textColor);
	
	
	Color color(high, .75 * high, 0., 1.);
	if((**shipIt).Weapons()[index].IsTurret())
		color = Color(0., .75 * high, high, 1.);
	
	Color black(0., 1.);
	Point from(pos.X() - 5., pos.Y() + .5 * font.Height());
	Point mid(hardpoint.X(), from.Y());
	
	LineShader::Draw(from, mid, 3.5, black);
	LineShader::Draw(mid, hardpoint, 3.5, black);
	LineShader::Draw(from, mid, 1.5, color);
	LineShader::Draw(mid, hardpoint, 1.5, color);
	
	int x = from.X() + 120;
	int y = from.Y();
	zones.emplace_back(x, y, 240, 20, index);
}



InfoPanel::ClickZone::ClickZone(int x, int y, int width, int height, int index)
	: left(x - width / 2), top(y - height / 2),
	right(x + width / 2), bottom(y + height / 2), index(index)
{
}



bool InfoPanel::ClickZone::Contains(int x, int y) const
{
	return (x >= left && y >= top && x < right && y < bottom);
}



int InfoPanel::ClickZone::Index() const
{
	return index;
}
