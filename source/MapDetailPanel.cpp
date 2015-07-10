/* MapDetailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapDetailPanel.h"

#include "Color.h"
#include "Command.h"
#include "Dialog.h"
#include "DotShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "MissionPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "Trade.h"
#include "UI.h"
#include "WrappedText.h"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace std;



MapDetailPanel::MapDetailPanel(PlayerInfo &player, int commodity, const System *system)
	: MapPanel(player, commodity, system), governmentY(0), tradeY(0), selectedPlanet(nullptr)
{
}



MapDetailPanel::MapDetailPanel(const MapPanel &panel)
	: MapPanel(panel), governmentY(0), tradeY(0), selectedPlanet(nullptr)
{
}



void MapDetailPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawInfo();
	DrawOrbits();
}



// Only override the ones you need; the default action is to return false.
bool MapDetailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN || key == 'i')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MissionPanel(*this));
	}
	else if(key == 'o')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapOutfitterPanel(*this));
	}
	else if(key == 's')
	{
		GetUI()->Pop(this);
		GetUI()->Push(new MapShipyardPanel(*this));
	}
	else if((key == SDLK_TAB || command.Has(Command::JUMP)) && player.Flagship())
	{
		bool hasJumpDrive = player.Flagship()->Attributes().Get("jump drive");
		const vector<const System *> &links =
			hasJumpDrive ? player.GetSystem()->Neighbors() : player.GetSystem()->Links();
		
		if(!player.HasTravelPlan() && !links.empty())
			Select(links.front());
		else if(player.TravelPlan().size() == 1 && !links.empty())
		{
			auto it = links.begin();
			for( ; it != links.end(); ++it)
				if(*it == player.TravelPlan().front())
					break;
			
			if(it != links.end())
				++it;
			if(it == links.end())
				it = links.begin();
			
			Select(*it);
		}
	}
	else if(key == SDLK_DOWN)
	{
		if(commodity < 0 || commodity == 9)
			commodity = 0;
		else
			++commodity;
	}
	else if(key == SDLK_UP)
	{
		if(commodity <= 0)
			commodity = 9;
		else
			--commodity;
	}
	else if(key == 'f')
		GetUI()->Push(new Dialog(
			this, &MapDetailPanel::DoFind, "Search for:"));
	else
		return false;
	
	return true;
}



bool MapDetailPanel::Click(int x, int y)
{
	{
		const Interface *interface = GameData::Interfaces().Get("map buttons");
		char key = interface->OnClick(Point(x + 250, y));
		// In the mission panel, the "Done" button in the button bar should be
		// ignored (and is not shown).
		if(key)
			return DoKey(key);
	}
	if(x < Screen::Left() + 160)
	{
		if(y >= tradeY && y < tradeY + 200)
		{
			commodity = (y - tradeY) / 20;
			return true;
		}
		else if(y < governmentY)
			commodity = -4;
		else if(y >= governmentY && y < governmentY + 20)
			commodity = -3;
		else
		{
			for(const auto &it : planetY)
				if(y >= it.second && y < it.second + 90)
				{
					selectedPlanet = it.first;
					if(y >= it.second + 50 && y < it.second + 70)
					{
						if(commodity == -1 && selectedPlanet->HasShipyard())
							ListShips();
						commodity = -1;
					}
					else if(y >= it.second + 70 && y < it.second + 90)
					{
						if(commodity == -2 && selectedPlanet->HasOutfitter())
							ListOutfits();
						commodity = -2;
					}
					return true;
				}
		}
	}
	else if(x >= Screen::Right() - 240 && y >= Screen::Bottom() - 240)
	{
		Point click = Point(x, y);
		selectedPlanet = nullptr;
		double distance = numeric_limits<double>::infinity();
		for(const auto &it : planets)
		{
			double d = click.Distance(it.second);
			if(d < distance)
			{
				distance = d;
				selectedPlanet = it.first;
			}
		}
		return true;
	}
	else if(y >= Screen::Bottom() - 40 && x >= Screen::Right() - 335 && x < Screen::Right() - 265)
	{
		// The user clicked the "done" button.
		return DoKey(SDLK_d);
	}
	else if(y >= Screen::Bottom() - 40 && x >= Screen::Right() - 415 && x < Screen::Right() - 345)
	{
		// The user clicked the "missions" button.
		return DoKey(SDLK_PAGEDOWN);
	}
	
	MapPanel::Click(x, y);
	if(selectedPlanet && selectedPlanet->GetSystem() != selectedSystem)
		selectedPlanet = nullptr;
	return true;
}



void MapDetailPanel::DoFind(const string &text)
{
	const Planet *planet = Find(text);
	if(planet)
		selectedPlanet = planet;
}



void MapDetailPanel::DrawInfo() const
{
	Color dimColor(.1, 0.);
	Color closeColor(.6, .6);
	Color farColor(.3, .3);
	
	Point uiPoint(Screen::Left() + 100., Screen::Top() + 45.);
	
	// System sprite goes from 0 to 90.
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");
	SpriteShader::Draw(systemSprite, uiPoint);
	
	const Font &font = FontSet::Get(14);
	string systemName = player.KnowsName(selectedSystem) ?
		selectedSystem->Name() : "Unexplored System";
	font.Draw(systemName, uiPoint + Point(-90., -7.), closeColor);
	
	governmentY = uiPoint.Y() + 10.;
	string gov = player.HasVisited(selectedSystem) ?
		selectedSystem->GetGovernment()->GetName() : "Unknown Government";
	font.Draw(gov, uiPoint + Point(-90., 13.), (commodity == -3) ? closeColor : farColor);
	if(commodity == -3)
		PointerShader::Draw(uiPoint + Point(-90., 20.), Point(1., 0.),
			10., 10., 0., closeColor);
	
	uiPoint.Y() += 105.;
	
	planetY.clear();
	if(player.HasVisited(selectedSystem))
	{
		const Sprite *planetSprite = SpriteSet::Get("ui/map planet");
		for(const StellarObject &object : selectedSystem->Objects())
			if(object.GetPlanet())
			{
				SpriteShader::Draw(planetSprite, uiPoint);
				planetY[object.GetPlanet()] = uiPoint.Y() - 50;
			
				font.Draw(object.GetPlanet()->Name(),
					uiPoint + Point(-70., -42.),
					object.GetPlanet() == selectedPlanet ? closeColor : farColor);
				font.Draw("Space Port",
					uiPoint + Point(-60., -22.),
					object.GetPlanet()->HasSpaceport() ? closeColor : dimColor);
				font.Draw("Shipyard",
					uiPoint + Point(-60., -2.),
					object.GetPlanet()->HasShipyard() ? closeColor : dimColor);
				if(commodity == -1)
					PointerShader::Draw(uiPoint + Point(-60., 5.), Point(1., 0.),
						10., 10., 0., closeColor);
				font.Draw("Outfitter",
					uiPoint + Point(-60., 18.),
					object.GetPlanet()->HasOutfitter() ? closeColor : dimColor);
				if(commodity == -2)
					PointerShader::Draw(uiPoint + Point(-60., 25.), Point(1., 0.),
						10., 10., 0., closeColor);
			
				uiPoint.Y() += 110.;
			}
	}
	
	uiPoint.Y() += 55.;
	tradeY = uiPoint.Y() - 95.;
	
	// Trade sprite goes from 310 to 540.
	const Sprite *tradeSprite = SpriteSet::Get("ui/map trade");
	SpriteShader::Draw(tradeSprite, uiPoint);
	
	uiPoint.X() -= 90.;
	uiPoint.Y() -= 97.;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		bool isSelected = false;
		if(static_cast<unsigned>(this->commodity) < GameData::Commodities().size())
			isSelected = (&commodity == &GameData::Commodities()[this->commodity]);
		Color &color = isSelected ? closeColor : farColor;
		
		font.Draw(commodity.name, uiPoint, color);
		
		if(player.HasVisited(selectedSystem))
		{
			string price = to_string(selectedSystem->Trade(commodity.name));
			Point pos = uiPoint + Point(140. - font.Width(price), 0.);
			font.Draw(price, pos, color);
		}
		
		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10., 10., 0., color);
		
		uiPoint.Y() += 20.;
	}
	
	if(selectedPlanet)
	{
		const Sprite *panelSprite = SpriteSet::Get("ui/description panel");
		Point pos(Screen::Right() - .5 * panelSprite->Width(),
			Screen::Top() + .5 * panelSprite->Height());
		SpriteShader::Draw(panelSprite, pos);
		
		WrappedText text;
		text.SetFont(FontSet::Get(14));
		text.SetAlignment(WrappedText::JUSTIFIED);
		text.SetWrapWidth(480);
		text.Wrap(selectedPlanet->Description());
		text.Draw(Point(Screen::Right() - 500, Screen::Top() + 20), closeColor);
	}
	
	// Draw the buttons.
	Information info;
	info.SetCondition("is ports");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info, Point(-250., 0.));
}



void MapDetailPanel::DrawOrbits() const
{
	// Draw the planet orbits in the currently selected system.
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits");
	Point orbitCenter(Screen::Right() - 130, Screen::Bottom() - 140);
	SpriteShader::Draw(orbitSprite, orbitCenter);
	orbitCenter.Y() += 10.;
	
	if(!selectedSystem || !player.HasVisited(selectedSystem))
		return;
	
	const Font &font = FontSet::Get(14);
	
	// Figure out what the largest orbit in this system is.
	double maxDistance = 0.;
	for(const StellarObject &object : selectedSystem->Objects())
		maxDistance = max(maxDistance, object.Position().Length() + object.Radius());
	
	// 2400 -> 120.
	double scale = .03;
	maxDistance *= scale;
	
	if(maxDistance > 120.)
		scale *= 120. / maxDistance;
	
	static const Color habitColor[7] = {
		Color(.4, 0., 0., 0.),
		Color(.3, .3, 0., 0.),
		Color(0., .4, 0., 0.),
		Color(0., .3, .4, 0.),
		Color(0., 0., .5, 0.),
		Color(.2, .2, .2, 0.),
		Color(1., 1., 1., 0.)
	};
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point parentPos;
		int habit = 5;
		if(object.Parent() >= 0)
			parentPos = selectedSystem->Objects()[object.Parent()].Position();
		else
		{
			double warmth = object.Distance() / selectedSystem->HabitableZone();
			habit = (warmth > .5) + (warmth > .8) + (warmth > 1.2) + (warmth > 2.0);
		}
		
		double radius = object.Distance() * scale;
		DotShader::Draw(orbitCenter + parentPos * scale,
			radius + .7, radius - .7,
			habitColor[habit]);
		
		if(selectedPlanet && object.GetPlanet() == selectedPlanet)
			DotShader::Draw(orbitCenter + object.Position() * scale,
				object.Radius() * scale + 5., object.Radius() * scale + 4.,
				habitColor[6]);
	}
	
	planets.clear();
	static const Color planetColor[4] = {
		Color(1., 1., 1., 1.),
		Color(.3, .3, .3, 1.),
		Color(0., .8, 1., 1.),
		Color(.8, .4, .2, 1.)
	};
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point pos = orbitCenter + object.Position() * scale;
		if(object.GetPlanet())
			planets[object.GetPlanet()] = pos;
		DotShader::Draw(pos,
			object.Radius() * scale + 1., 0.,
				planetColor[!object.IsStar() + (object.GetPlanet() != nullptr)
					+ (object.GetPlanet() && !object.GetPlanet()->CanLand())]);
	}
	
	// Draw the name of the selected planet.
	const string &name = selectedPlanet ? selectedPlanet->Name() : selectedSystem->Name();
	int width = font.Width(name);
	width = (width / 2) + 65;
	Point namePos(Screen::Right() - width - 5., Screen::Bottom() - 267.);
	Color nameColor(.6, .6);
	font.Draw(name, namePos, nameColor);
}



void MapDetailPanel::ListShips() const
{
	if(!selectedPlanet)
		return;
	
	// First, count how many planets have each ship.
	map<const Ship *, int> count;
	for(const auto &it : GameData::Planets())
		for(const Ship *ship : it.second.Shipyard())
			++count[ship];
	
	vector<pair<int, const Ship *>> list;
	for(const Ship *ship : selectedPlanet->Shipyard())
		list.emplace_back(count[ship], ship);
	
	sort(list.begin(), list.end());
	ostringstream out;
	out << "Ships for sale here:";
	for(unsigned i = 0; i < 10 + (list.size() == 11) && i < list.size(); ++i)
		out << '\n' << list[i].second->ModelName();
	if(list.size() > 11)
		out << "\n...and " << list.size() - 10 << " others.";
	GetUI()->Push(new Dialog(out.str()));
}



void MapDetailPanel::ListOutfits() const
{
	if(!selectedPlanet)
		return;
	
	// First, count how many planets have each ship.
	map<const Outfit *, int> count;
	for(const auto &it : GameData::Planets())
		for(const Outfit *outfit : it.second.Outfitter())
			++count[outfit];
	
	vector<pair<int, const Outfit *>> list;
	for(const Outfit *ship : selectedPlanet->Outfitter())
		list.emplace_back(count[ship], ship);
	
	sort(list.begin(), list.end());
	ostringstream out;
	out << "Outfits for sale here:";
	for(unsigned i = 0; i < 18 + (list.size() == 19) && i < list.size(); ++i)
		out << '\n' << list[i].second->Name();
	if(list.size() > 19)
		out << "\n...and " << list.size() - 18 << " others.";
	GetUI()->Push(new Dialog(out.str()));
}
