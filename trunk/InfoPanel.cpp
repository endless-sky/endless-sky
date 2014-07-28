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
	Color turretLine(0., .6, .8, 1.);
	Color gunLine(.8, .6, 0., 1.);
	pos = Point(10., 250.);
	for(const Armament::Weapon &weapon : ship.Weapons())
		if(weapon.IsTurret())
		{
			font.Draw(weapon.GetOutfit()->Name(), pos, dim);
			
			Point from(pos.X() - 5., pos.Y() + .5 * font.Height());
			Point to = shipCenter + (2. * scale) * weapon.GetPoint();
			Point mid(to.X(), from.Y());
			LineShader::Draw(from, mid, 3.5, black);
			LineShader::Draw(mid, to, 3.5, black);
			LineShader::Draw(from, mid, 1.5, turretLine);
			LineShader::Draw(mid, to, 1.5, turretLine);
			
			pos.Y() -= 20.;
		}
	if(pos.Y() != 250.)
		pos.Y() -= 10.;
	for(const Armament::Weapon &weapon : ship.Weapons())
		if(!weapon.IsTurret())
		{
			font.Draw(weapon.GetOutfit()->Name(), pos, dim);
			
			Point from(pos.X() - 5., pos.Y() + .5 * font.Height());
			Point to = shipCenter + (2. * scale) * weapon.GetPoint();
			Point mid(to.X(), from.Y());
			LineShader::Draw(from, mid, 3.5, black);
			LineShader::Draw(mid, to, 3.5, black);
			LineShader::Draw(from, mid, 1.5, gunLine);
			LineShader::Draw(mid, to, 1.5, gunLine);
			
			pos.Y() -= 20.;
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
	
	return true;
}



void InfoPanel::UpdateInfo()
{
	if(shipIt == player.Ships().end())
		return;
	
	const Ship &ship = **shipIt;
	info.Update(ship);
	
	outfits.clear();
	for(const auto &it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);
}
