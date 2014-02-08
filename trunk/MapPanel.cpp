/* MapPanel.cpp
Michael Zahniser, 2 Nov 2013

Function definitions for the MapPanel class.
*/

#include "MapPanel.h"

#include "DotShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "LineShader.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Screen.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "Trade.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace std;



MapPanel::MapPanel(const GameData &data, PlayerInfo &player, int commodity)
	: data(data), current(player.GetShip()->GetSystem()),
	selected(player.GetShip()->GetSystem()), player(player),
	tradeY(0), commodity(commodity), selectedPlanet(nullptr)
{
	vector<const System *> edge{current};
	distance[current] = 0;
	
	for(int steps = 1; !edge.empty(); ++steps)
	{
		vector<const System *> newEdge;
		for(const System *system : edge)
			for(const System *link : system->Links())
			{
				auto it = distance.find(link);
				if(it != distance.end())
					continue;
			
				distance[link] = steps;
				newEdge.push_back(link);
			}
		newEdge.swap(edge);
	}
	
	center = Point(0., 0.) - current->Position();
}



void MapPanel::Draw() const
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Sprite *galaxy = SpriteSet::Get("ui/galaxy");
	SpriteShader::Draw(galaxy, center);
	
	const Set<System> &systems = data.Systems();
	
	static const float dimColor[4] = {.1f, .1f, .1f, 0.f};
	DotShader::Draw(current->Position() + center, 100.5, 99.5, dimColor);
	
	static const float selectedColor[4] = {1.f, 1.f, 1.f, 1.f};
	static const float closeColor[4] = {.6f, .6f, .6f, .6f};
	static const float farColor[4] = {.3f, .3f, .3f, .3f};
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
		if(!player.HasSeen(&system))
			continue;
		
		float color[4] = {.2f, .2f, .2f, .2f};
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
					value = system.HasShipyard();
				else if(commodity == -2)
					value = system.HasOutfitter();
				color[0] = .6f * (1.f - max(-value, 0.f));
				color[1] = .6f * (1.f - abs(value));
				color[2] = .6f * (1.f - max(value, 0.f));
				color[3] = .4f;
			}
			else
			{
				for(int i = 0; i < 3; ++i)
					color[i] = .6f * system.GetGovernment().GetColor().Get()[i];
				color[3] = .4;
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
		
		float color[4] = {.4f, .4f, .0f, .0f};
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
	
	Point uiPoint(-Screen::Width() / 2 + 100., -Screen::Height() / 2 + 45.);
	
	// System sprite goes from 0 to 90.
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");
	SpriteShader::Draw(systemSprite, uiPoint);
	font.Draw(selected->Name(),
		uiPoint + Point(-90., -7.),
		closeColor);
	governmentY = uiPoint.Y() + 10.;
	font.Draw(selected->GetGovernment().GetName(),
		uiPoint + Point(-90., 13.),
		(commodity == -3) ? closeColor : farColor);
	if(commodity == -3)
		PointerShader::Draw(uiPoint + Point(-90., 20.), Point(1., 0.),
			10., 10., 0., closeColor);
	
	uiPoint.Y() += 105.;
	
	planetY.clear();
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
		if(this->commodity >= 0 && this->commodity < data.Commodities().size())
			isSelected = (&commodity == &data.Commodities()[this->commodity]);
		const float *color = isSelected ? closeColor : farColor;
		
		font.Draw(commodity.name, uiPoint, color);
		
		string price = to_string(selected->Trade(commodity.name));
		Point pos = uiPoint + Point(140. - font.Width(price), 0.);
		font.Draw(price, pos, color);
		
		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10., 10., 0., color);
		
		uiPoint.Y() += 20.;
	}
	
	// Draw the planet orbits in the currently selected system.
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits");
	Point orbitCenter(Screen::Width() / 2 - 130, Screen::Height() / 2 - 140);
	SpriteShader::Draw(orbitSprite, orbitCenter);
	orbitCenter.Y() += 10.;
	
	// Figure out wht the largest orbit in this system is.
	double maxDistance = 0.;
	for(const StellarObject &object : selected->Objects())
		maxDistance = max(maxDistance, object.Position().Length() + object.Radius());
	
	// 2400 -> 120.
	static const double scale = .03;
	maxDistance *= scale;
	maxDistance -= 120.;
	
	if(maxDistance > 0)
		orbitCenter += Point(maxDistance, maxDistance);
	
	static const float habitColor[7][4] = {
		{.4, 0., 0., 0.},
		{.3, .3, 0., 0.},
		{0., .4, 0., 0.},
		{0., .3, .4, 0.},
		{0., 0., .5, 0.},
		{.2, .2, .2, 0.},
		{1., 1., 1., 0.}};
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
	static const float planetColor[3][4] = {
		{1., 1., 1., 1.},
		{.3, .3, .3, 1.},
		{0., .8, 1., 1.}};
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
	Point namePos(Screen::Width() / 2 - width - 5., Screen::Height() / 2 - 267.);
	font.Draw(name, namePos, closeColor);
}



// Only override the ones you need; the default action is to return false.
bool MapPanel::KeyDown(SDLKey key, SDLMod mod)
{
	if(key == SDLK_ESCAPE)
		Pop(this);
	
	return true;
}



bool MapPanel::Click(int x, int y)
{
	if(x < 160 - Screen::Width() / 2)
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
	else if(x >= Screen::Width() / 2 - 240 && y >= Screen::Height() / 2 - 240)
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
		auto it = distance.find(system);
		if(it != distance.end())
		{
			player.ClearTravel();
			int steps = it->second;
			while(system != current)
			{
				player.AddTravel(system);
				for(const System *link : system->Links())
				{
					auto lit = distance.find(link);
					if(lit != distance.end() && lit->second < steps)
					{
						steps = lit->second;
						system = link;
						break;
					}
				}
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
