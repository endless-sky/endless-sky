/* MapPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapPanel.h"

#include "DotShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "LineShader.h"
#include "Mission.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "Trade.h"
#include "UI.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace std;



MapPanel::MapPanel(const GameData &data, PlayerInfo &player, int commodity)
	: data(data), current(player.GetShip()->GetSystem()),
	selected(player.GetShip()->GetSystem()), player(player), distance(player),
	tradeY(0), commodity(commodity), selectedPlanet(nullptr)
{
	SetIsFullScreen(true);
	
	// Special case: any systems which have not been seen but which are the
	// destination of a mission, should be shown in the map.
	for(const Mission &mission : player.AvailableJobs())
		destinations.insert(mission.Destination()->GetSystem());
	for(const Mission &mission : player.Missions())
		destinations.insert(mission.Destination()->GetSystem());
	
	center = Point(0., 0.) - current->Position();
}



void MapPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Sprite *galaxy = SpriteSet::Get("ui/galaxy");
	SpriteShader::Draw(galaxy, center);
	
	const Set<System> &systems = data.Systems();
	
	Color dimColor(.1, 0.);
	DotShader::Draw(current->Position() + center, 100.5, 99.5, dimColor);
	
	Color closeColor(.6, .6);
	Color farColor(.3, .3);
	for(const auto &it : systems)
	{
		const System *system = &it.second;
		if(!player.HasSeen(system))
			continue;
		
		for(const System *link : system->Links())
			if(link < system || !player.HasSeen(link))
			{
				if(!player.HasVisited(system) && !player.HasVisited(link))
					continue;
				
				Point from = system->Position() + center;
				Point to = link->Position() + center;
				Point unit = (from - to).Unit() * 7.;
				from -= unit;
				to += unit;
				LineShader::Draw(from, to, 1.2,
					(system == current || link == current) ? closeColor : farColor);
			}
	}
	for(const auto &it : systems)
	{
		const System &system = it.second;
		if(!player.HasSeen(&system) && destinations.find(&system) == destinations.end())
			continue;
		
		Color color(.2, .2);
		if(system.IsInhabited() && player.HasVisited(&system))
		{
			if(commodity != -3)
			{
				float value = 0.f;
				if(commodity >= 0)
				{
					const Trade::Commodity &com = data.Commodities()[commodity];
					value = (2.f * (system.Trade(com.name) - com.low))
						/ (com.high - com.low) - 1.f;
				}
				else if(commodity == -1)
					value = system.HasShipyard() * 2 - 1;
				else if(commodity == -2)
					value = system.HasOutfitter() * 2 - 1;
				// Color the systems with a gradient from blue to cyan to gold.
				if(value < 0.f)
					color = Color(
						.12 + .12 * value,
						.48 + .36 * value,
						.48 - .12 * value,
						.4f);
				else
					color = Color(
						.12 + .48 * value,
						.48,
						.48 - .48 * value,
						.4f);
			}
			else
			{
				color = Color(
					.6f * system.GetGovernment().GetColor().Get()[0],
					.6f * system.GetGovernment().GetColor().Get()[1],
					.6f * system.GetGovernment().GetColor().Get()[2],
					.4f);
			}
		}
		
		DotShader::Draw(system.Position() + center, 6., 3.5, color);
		if(&system == selected)
			DotShader::Draw(system.Position() + center, 10., 9., color);
	}
	const System *previous = current;
	for(int i = player.TravelPlan().size() - 1; i >= 0; --i)
	{
		const System *next = player.TravelPlan()[i];
		
		Point from = next->Position() + center;
		Point to = previous->Position() + center;
		Point unit = (from - to).Unit() * 7.;
		from -= unit;
		to += unit;
		
		Color color(.4, .4, 0., 0.);
		LineShader::Draw(from, to, 3., color);
		
		previous = next;
	}
	const Font &font = FontSet::Get(14);
	Point offset(6., -.5 * font.Height());
	for(const auto &it : systems)
	{
		const System &system = it.second;
		if(!player.HasVisited(&system))
			continue;
		
		font.Draw(system.Name(), system.Position() + offset + center,
			(&system == current) ? closeColor : farColor);
	}
	
	Point uiPoint(Screen::Left() + 100., Screen::Top() + 45.);
	
	// System sprite goes from 0 to 90.
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");
	SpriteShader::Draw(systemSprite, uiPoint);
	
	string systemName = player.HasVisited(selected) ?
		selected->Name() : "Unexplored System";
	font.Draw(systemName, uiPoint + Point(-90., -7.), closeColor);
	
	governmentY = uiPoint.Y() + 10.;
	string gov = player.HasVisited(selected) ?
		selected->GetGovernment().GetName() : "Unknown Government";
	font.Draw(gov, uiPoint + Point(-90., 13.), (commodity == -3) ? closeColor : farColor);
	if(commodity == -3)
		PointerShader::Draw(uiPoint + Point(-90., 20.), Point(1., 0.),
			10., 10., 0., closeColor);
	
	uiPoint.Y() += 105.;
	
	planetY.clear();
	if(player.HasVisited(selected))
	{
		const Sprite *planetSprite = SpriteSet::Get("ui/map planet");
		for(const StellarObject &object : selected->Objects())
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
	for(const Trade::Commodity &commodity : data.Commodities())
	{
		bool isSelected = false;
		if(static_cast<unsigned>(this->commodity) < data.Commodities().size())
			isSelected = (&commodity == &data.Commodities()[this->commodity]);
		Color &color = isSelected ? closeColor : farColor;
		
		font.Draw(commodity.name, uiPoint, color);
		
		if(player.HasVisited(selected))
		{
			string price = to_string(selected->Trade(commodity.name));
			Point pos = uiPoint + Point(140. - font.Width(price), 0.);
			font.Draw(price, pos, color);
		}
		
		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10., 10., 0., color);
		
		uiPoint.Y() += 20.;
	}
	
	// Draw a pointer for each active or current mission.
	map<const System *, Angle> angle;
	for(const Mission &mission : player.AvailableJobs())
	{
		const System *system = mission.Destination()->GetSystem();
		Angle a = (angle[system] += Angle(30.));
		Color color(.2, 1., 0., 1.);
		PointerShader::Draw(system->Position() + center, a.Unit(), 8., 15., -6., color);
	}
	for(const Mission &mission : player.Missions())
	{
		const System *system = mission.Destination()->GetSystem();
		Angle a = (angle[system] += Angle(30.));
		Color color(.2, .8, 1., 1.);
		PointerShader::Draw(system->Position() + center, a.Unit(), 8., 15., -6., color);
	}
	
	// Draw the "Done" button.
	const Sprite *buttonSprite = SpriteSet::Get("ui/dialog cancel");
	Point buttonCenter(Screen::Right() - 300, Screen::Bottom() - 25);
	SpriteShader::Draw(buttonSprite, buttonCenter);
	static const string DONE = "Done";
	buttonCenter.X() -= .5 * font.Width(DONE);
	buttonCenter.Y() -= .5 * font.Height();
	font.Draw(DONE, buttonCenter, Color(.8, 0.));
	
	// Draw the planet orbits in the currently selected system.
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits");
	Point orbitCenter(Screen::Right() - 130, Screen::Bottom() - 140);
	SpriteShader::Draw(orbitSprite, orbitCenter);
	orbitCenter.Y() += 10.;
	
	if(!player.HasVisited(selected))
		return;
	
	// Figure out what the largest orbit in this system is.
	double maxDistance = 0.;
	for(const StellarObject &object : selected->Objects())
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
	for(const StellarObject &object : selected->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point parentPos;
		int habit = 5;
		if(object.Parent() >= 0)
			parentPos = selected->Objects()[object.Parent()].Position();
		else
		{
			double warmth = object.Distance() / selected->HabitableZone();
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
	static const Color planetColor[3] = {
		Color(1., 1., 1., 1.),
		Color(.3, .3, .3, 1.),
		Color(0., .8, 1., 1.)
	};
	for(const StellarObject &object : selected->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point pos = orbitCenter + object.Position() * scale;
		if(object.GetPlanet())
			planets[object.GetPlanet()] = pos;
		DotShader::Draw(pos,
			object.Radius() * scale + 1., 0.,
				planetColor[!object.IsStar() + (object.GetPlanet() != nullptr)]);
	}
	
	// Draw the name of the selected planet.
	const string &name = selectedPlanet ? selectedPlanet->Name() : selected->Name();
	int width = font.Width(name);
	width = (width / 2) + 65;
	Point namePos(Screen::Right() - width - 5., Screen::Bottom() - 267.);
	font.Draw(name, namePos, closeColor);
}



// Only override the ones you need; the default action is to return false.
bool MapPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == data.Keys().Get(Key::MAP) || key == 'd')
		GetUI()->Pop(this);
	else
		return false;
	
	return true;
}



bool MapPanel::Click(int x, int y)
{
	if(x < Screen::Left() + 160)
	{
		if(y >= tradeY && y < tradeY + 200)
		{
			commodity = (y - tradeY) / 20;
			return true;
		}
		else if(y >= governmentY && y < governmentY + 20)
			commodity = -3;
		else
		{
			for(const auto &it : planetY)
				if(y >= it.second && y < it.second + 90)
				{
					selectedPlanet = it.first;
					if(y >= it.second + 50 && y < it.second + 70)
						commodity = -1;
					else if(y >= it.second + 70 && y < it.second + 90)
						commodity = -2;
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
		return KeyDown(SDLK_d, KMOD_NONE);
	}
	// Figure out if a system was clicked on.
	Point click = Point(x, y) - center;
	const System *system = nullptr;
	for(const auto &it : data.Systems())
		if(click.Distance(it.second.Position()) < 10.)
		{
			selectedPlanet = nullptr;
			system = &it.second;
			break;
		}
	if(system)
	{
		selected = system;
		if(distance.HasRoute(system))
		{
			player.ClearTravel();
			while(system != current)
			{
				player.AddTravel(system);
				system = distance.Route(system);
			}
		}
	}
	
	return true;
}



bool MapPanel::Drag(int dx, int dy)
{
	center += Point(dx, dy);
	return true;
}
