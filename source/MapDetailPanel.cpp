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
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "MissionPanel.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "RingShader.h"
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
#include <cmath>
#include <set>
#include <sstream>
#include <vector>

using namespace std;



MapDetailPanel::MapDetailPanel(PlayerInfo &player, int commodity, const System *system)
	: MapPanel(player, commodity, system), governmentY(0), tradeY(0), selectedPlanet(nullptr)
{
}



MapDetailPanel::MapDetailPanel(PlayerInfo &player, int *commodity)
	: MapDetailPanel(player, *commodity)
{
	tradeCommodity = commodity;
}



MapDetailPanel::MapDetailPanel(const MapPanel &panel)
	: MapPanel(panel), governmentY(0), tradeY(0), selectedPlanet(nullptr)
{
	// Don't use the "special" coloring in this view.
	if(commodity == SHOW_SPECIAL)
		commodity = SHOW_REPUTATION;
}



void MapDetailPanel::Draw()
{
	MapPanel::Draw();
	
	DrawKey();
	DrawInfo();
	DrawOrbits();
}



// Only override the ones you need; the default action is to return false.
bool MapDetailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(command.Has(Command::MAP) || key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
		// Toggle to the next link connected to the "source" system. If the
		// shift key is down, the source is the end of the travel plan; otherwise
		// it is one step before the end.
		vector<const System *> &plan = player.TravelPlan();
		const System *source = plan.empty() ? player.GetSystem() : plan.front();
		const System *next = nullptr;
		Point previousUnit = Point(0., -1.);
		if(!plan.empty() && !(mod & KMOD_SHIFT))
		{
			previousUnit = plan.front()->Position();
			plan.erase(plan.begin());
			next = source;
			source = plan.empty() ? player.GetSystem() : plan.front();
			previousUnit = (previousUnit - source->Position()).Unit();
		}
		Point here = source->Position();
		
		// Depending on whether the flagship has a jump drive, the possible links
		// we can travel along are different:
		bool hasJumpDrive = player.Flagship()->Attributes().Get("jump drive");
		const vector<const System *> &links = hasJumpDrive ? source->Neighbors() : source->Links();
		
		double bestAngle = 2. * PI;
		for(const System *it : links)
		{
			if(!player.HasSeen(it))
				continue;
			if(!(hasJumpDrive || player.HasVisited(it) || player.HasVisited(source)))
				continue;
			
			Point unit = (it->Position() - here).Unit();
			double angle = acos(unit.Dot(previousUnit));
			if(unit.Cross(previousUnit) >= 0.)
				angle = 2. * PI - angle;
			
			if(angle <= bestAngle)
			{
				next = it;
				bestAngle = angle;
			}
		}
		if(next)
		{
			plan.insert(plan.begin(), next);
			Select(next);
		}
	}
	else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && player.HasTravelPlan())
	{
		vector<const System *> &plan = player.TravelPlan();
		plan.erase(plan.begin());
		Select(plan.empty() ? player.GetSystem() : plan.front());
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
	else if(key == '+' || key == '=')
		ZoomMap();
	else if(key == '-')
		UnzoomMap();
	else
		return false;
	
	return true;
}



bool MapDetailPanel::Click(int x, int y)
{
	if(x < Screen::Left() + 160)
	{
		if(y >= tradeY && y < tradeY + 200)
		{
			commodity = (y - tradeY) / 20;
			return true;
		}
		else if(y < governmentY)
			commodity = SHOW_REPUTATION;
		else if(y >= governmentY && y < governmentY + 20)
			commodity = SHOW_GOVERNMENT;
		else
		{
			for(const auto &it : planetY)
				if(y >= it.second && y < it.second + 110)
				{
					selectedPlanet = it.first;
					if(y >= it.second + 50 && y < it.second + 70)
					{
						if(commodity == SHOW_SHIPYARD && selectedPlanet->HasShipyard())
							ListShips();
						commodity = SHOW_SHIPYARD;
					}
					else if(y >= it.second + 70 && y < it.second + 90)
					{
						if(commodity == SHOW_OUTFITTER && selectedPlanet->HasOutfitter())
							ListOutfits();
						commodity = SHOW_OUTFITTER;
					}
					else if(y >= it.second + 90 && y < it.second + 110)
						commodity = SHOW_VISITED;
					return true;
				}
		}
	}
	else if(x >= Screen::Right() - 240 && y >= Screen::Top() + 280 && y <= Screen::Top() + 520)
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



void MapDetailPanel::DrawKey() const
{
	const Sprite *back = SpriteSet::Get("ui/map key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));
	
	Color bright(.6, .6);
	Color dim(.3, .3);
	const Font &font = FontSet::Get(14);
	
	Point pos(Screen::Left() + 10., Screen::Bottom() - 7. * 20. + 5.);
	Point headerOff(-5., -.5 * font.Height());
	Point textOff(10., -.5 * font.Height());
	
	static const string HEADER[] = {
		"Trade prices:",
		"Ships for sale:",
		"Outfits for sale:",
		"You have visited:",
		"", // Special should never be active in this mode.
		"Government:",
		"System:"
	};
	const string &header = HEADER[-min(0, max(-6, commodity))];
	font.Draw(header, pos + headerOff, bright);
	pos.Y() += 20.;
	
	if(commodity >= 0)
	{
		const vector<Trade::Commodity> &commodities = GameData::Commodities();
		const auto &range = commodities[commodity];
		if(static_cast<unsigned>(commodity) >= commodities.size())
			return;
		
		for(int i = 0; i <= 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(i * (2. / 3.) - 1.));
			int price = range.low + ((range.high - range.low) * i) / 3;
			font.Draw(Format::Number(price), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity >= SHOW_OUTFITTER)
	{
		static const string LABEL[2][4] = {
			{"None", "1", "5", "10+"},
			{"None", "1", "30", "60+"}};
		static const double VALUE[4] = {-1., 0., .5, 1.};
		
		for(int i = 0; i < 4; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
			font.Draw(LABEL[commodity == SHOW_OUTFITTER][i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_VISITED)
	{
		static const string LABEL[3] = {
			"All planets",
			"Some",
			"None"
		};
		for(int i = 0; i < 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(1 - i));
			font.Draw(LABEL[i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_GOVERNMENT)
	{
		vector<pair<double, const Government *>> distances;
		for(const auto &it : closeGovernments)
			distances.emplace_back(it.second, it.first);
		sort(distances.begin(), distances.end());
		for(unsigned i = 0; i < 4 && i < distances.size(); ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, GovernmentColor(distances[i].second));
			font.Draw(distances[i].second->GetName(), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_REPUTATION)
	{
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(1e-1, true, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(1e2, true, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(1e4, true, false));
		font.Draw("Friendly", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(-1e-1, false, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(-1e2, false, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(-1e4, false, false));
		font.Draw("Hostile", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, false));
		font.Draw("Restricted", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, true));
		font.Draw("Dominated", pos + textOff, dim);
		pos.Y() += 20.;
	}
	
	RingShader::Draw(pos, OUTER, INNER, UninhabitedColor());
	font.Draw("Uninhabited", pos + textOff, dim);
	pos.Y() += 20.;
	
	RingShader::Draw(pos, OUTER, INNER, UnexploredColor());
	font.Draw("Unexplored", pos + textOff, dim);
}



void MapDetailPanel::DrawInfo()
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
	font.Draw(gov, uiPoint + Point(-90., 13.), (commodity == SHOW_GOVERNMENT) ? closeColor : farColor);
	if(commodity == SHOW_GOVERNMENT)
		PointerShader::Draw(uiPoint + Point(-90., 20.), Point(1., 0.),
			10., 10., 0., closeColor);
	
	uiPoint.Y() += 115.;
	
	planetY.clear();
	if(player.HasVisited(selectedSystem))
	{
		set<const Planet *> shown;
		const Sprite *planetSprite = SpriteSet::Get("ui/map planet");
		for(const StellarObject &object : selectedSystem->Objects())
			if(object.GetPlanet())
			{
				// Allow the same "planet" to appear multiple times in one system.
				auto it = shown.find(object.GetPlanet());
				if(it != shown.end())
					continue;
				shown.insert(object.GetPlanet());
				
				SpriteShader::Draw(planetSprite, uiPoint);
				planetY[object.GetPlanet()] = uiPoint.Y() - 60;
			
				font.Draw(object.Name(),
					uiPoint + Point(-70., -52.),
					object.GetPlanet() == selectedPlanet ? closeColor : farColor);
				
				font.Draw("Space Port",
					uiPoint + Point(-60., -32.),
					object.GetPlanet()->HasSpaceport() ? closeColor : dimColor);
				
				font.Draw("Shipyard",
					uiPoint + Point(-60., -12.),
					object.GetPlanet()->HasShipyard() ? closeColor : dimColor);
				if(commodity == SHOW_SHIPYARD)
					PointerShader::Draw(uiPoint + Point(-60., -5.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				font.Draw("Outfitter",
					uiPoint + Point(-60., 8.),
					object.GetPlanet()->HasOutfitter() ? closeColor : dimColor);
				if(commodity == SHOW_OUTFITTER)
					PointerShader::Draw(uiPoint + Point(-60., 15.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				bool hasVisited = player.HasVisited(object.GetPlanet());
				font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)",
					uiPoint + Point(-70., 28.),
					farColor);
				if(commodity == SHOW_VISITED)
					PointerShader::Draw(uiPoint + Point(-70., 35.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				uiPoint.Y() += 130.;
			}
	}
	
	uiPoint.Y() += 45.;
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
		
		string price;
		
		bool hasVisited = player.HasVisited(selectedSystem);
		if(hasVisited && selectedSystem->IsInhabited())
		{
			int value = selectedSystem->Trade(commodity.name);
			int localValue = (player.GetSystem() ? player.GetSystem()->Trade(commodity.name) : 0);
			if(!player.GetSystem() || player.GetSystem() == selectedSystem || !value || !localValue)
				price = to_string(value);
			else
			{
				value -= localValue;
				price += "(";
				if(value > 0)
					price += '+';
				price += to_string(value);
				price += ")";
			}
		}
		else
			price = (hasVisited ? "n/a" : "?");
		
		Point pos = uiPoint + Point(140. - font.Width(price), 0.);
		font.Draw(price, pos, color);
		
		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10., 10., 0., color);
		
		uiPoint.Y() += 20.;
	}
	
	if(selectedPlanet && !selectedPlanet->Description().empty() && player.HasVisited(selectedPlanet))
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
	if(ZoomIsMax())
		info.SetCondition("max zoom");
	if(ZoomIsMin())
		info.SetCondition("min zoom");
	const Interface *interface = GameData::Interfaces().Get("map buttons");
	interface->Draw(info, this);
}



void MapDetailPanel::DrawOrbits() const
{
	// Draw the planet orbits in the currently selected system.
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits");
	Point orbitCenter(Screen::Right() - 120, Screen::Top() + 430);
	SpriteShader::Draw(orbitSprite, orbitCenter - Point(5., 0.));
	
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
	
	if(maxDistance > 115.)
		scale *= 115. / maxDistance;
	
	static const Color habitColor[7] = {
		Color(.4, .2, .2, 1.),
		Color(.3, .3, 0., 1.),
		Color(0., .4, 0., 1.),
		Color(0., .3, .4, 1.),
		Color(.1, .2, .5, 1.),
		Color(.2, .2, .2, 1.),
		Color(1., 1., 1., 1.)
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
		RingShader::Draw(orbitCenter + parentPos * scale,
			radius + .7, radius - .7,
			habitColor[habit]);
		
		if(selectedPlanet && object.GetPlanet() == selectedPlanet)
			RingShader::Draw(orbitCenter + object.Position() * scale,
				object.Radius() * scale + 5., object.Radius() * scale + 4.,
				habitColor[6]);
	}
	
	planets.clear();
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point pos = orbitCenter + object.Position() * scale;
		if(object.GetPlanet())
			planets[object.GetPlanet()] = pos;
		
		RingShader::Draw(pos, object.Radius() * scale + 1., 0., object.TargetColor());
	}
	
	// Draw the name of the selected planet.
	const string &name = selectedPlanet ? selectedPlanet->Name() : selectedSystem->Name();
	int width = font.Width(name);
	width = (width / 2) + 65;
	Point namePos(Screen::Right() - width - 5., Screen::Top() + 293.);
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
