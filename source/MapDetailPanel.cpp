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
#include "Format.h"
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
	// Don't use the "special" coloring in this view.
	commodity = max(commodity, -4);
}



void MapDetailPanel::Draw() const
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
		"Government:",
		"System:"
	};
	const string &header = HEADER[-min(0, max(-4, commodity))];
	font.Draw(header, pos + headerOff, bright);
	pos.Y() += 20.;
	
	if(commodity >= 0)
	{
		const std::vector<Trade::Commodity> &commodities = GameData::Commodities();
		const auto &range = commodities[commodity];
		if(static_cast<unsigned>(commodity) >= commodities.size())
			return;
		
		for(int i = 0; i <= 3; ++i)
		{
			DotShader::Draw(pos, OUTER, INNER, MapColor(i * (2. / 3.) - 1.));
			int price = range.low + ((range.high - range.low) * i) / 3;
			font.Draw(Format::Number(price), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity >= -2)
	{
		static const string LABEL[2][4] = {
			{"None", "1", "5", "10+"},
			{"None", "1", "30", "60+"}};
		static const double VALUE[4] = {-1., 0., .5, 1.};
		
		for(int i = 0; i < 4; ++i)
		{
			DotShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
			font.Draw(LABEL[commodity == -2][i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == -3)
	{
		vector<pair<double, const Government *>> distances;
		for(const auto &it : closeGovernments)
			distances.emplace_back(it.second, it.first);
		sort(distances.begin(), distances.end());
		for(unsigned i = 0; i < 4 && i < distances.size(); ++i)
		{
			DotShader::Draw(pos, OUTER, INNER, GovernmentColor(distances[i].second));
			font.Draw(distances[i].second->GetName(), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else
	{
		DotShader::Draw(pos, OUTER, INNER, ReputationColor(1e-1, true, false));
		DotShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(1e2, true, false));
		DotShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(1e4, true, false));
		font.Draw("Friendly", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		DotShader::Draw(pos, OUTER, INNER, ReputationColor(-1e-1, true, false));
		DotShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(-1e2, true, false));
		DotShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(-1e4, true, false));
		font.Draw("Hostile", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		DotShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, false));
		font.Draw("Restricted", pos + textOff, dim);
		pos.Y() += 20.;
		
		DotShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, true));
		font.Draw("Dominated", pos + textOff, dim);
		pos.Y() += 20.;
	}
	
	DotShader::Draw(pos, OUTER, INNER, UninhabitedColor());
	font.Draw("Uninhabited", pos + textOff, dim);
	pos.Y() += 20.;
	
	DotShader::Draw(pos, OUTER, INNER, UnexploredColor());
	font.Draw("Unexplored", pos + textOff, dim);
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
			
				font.Draw(object.Name(),
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
	
	if (commodity >= 0)
	{
		uiPoint.X() += 110;
		uiPoint.Y() = tradeY + 96;
	
		const Sprite *keySprite = SpriteSet::Get("ui/thumb box right");
		SpriteShader::Draw(keySprite, uiPoint);
	
		uiPoint.X() -= 110;
		uiPoint.Y() = tradeY + 95;
	}
	
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
	
	if (commodity >= 0)
	{
		uiPoint.X() += 165.;
		uiPoint.Y() -= 145.;
		
		Color color = closeColor;
		font.Draw("Price Key", uiPoint, color);
		
		uiPoint.X() += 10.;
		uiPoint.Y() += 25.;
		
		for(unsigned i = 0; i < 4; ++i)
		{
			double value = 0.;
			string label;
			if(commodity >= 0)
			{
				const Trade::Commodity &com = GameData::Commodities()[commodity];
				value = (2. * (static_cast<double>(i)/3.)*(com.high - com.low))
					/ (com.high - com.low) - 1.;
				label = Format::Number(round(com.low + (com.high - com.low)*(static_cast<double>(i)/3.)));
			}
			//		else if(commodity == -1)
			//		{
			//			double size = 0;
			//			for(const StellarObject &object : system.Objects())
			//				if(object.GetPlanet())
			//					size += object.GetPlanet()->Shipyard().size();
			//			value = size ? min(10., size) / 10. : -1.;
			//		}
			//		else if(commodity == -2)
			//		{
			//			double size = 0;
			//			for(const StellarObject &object : system.Objects())
			//				if(object.GetPlanet())
			//					size += object.GetPlanet()->Outfitter().size();
			//			value = size ? min(60., size) / 60. : -1.;
			//		}
			else
			{
				value = 0.0;
				label = "";
			}
			
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
			
			DotShader::Draw(uiPoint, 6., 3.5, color);
			
			Color labelColor = closeColor;
			
			Point pos = uiPoint + Point(15., -8.);
			font.Draw(label, pos, labelColor);
			
			uiPoint.Y() += 17.;
		}
	}
	
	if(selectedPlanet && !selectedPlanet->Description().empty())
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
	static const Color planetColor[5] = {
		Color(1., 1., 1., 1.),
		Color(.3, .3, .3, 1.),
		Color(0., .8, 1., 1.),
		Color(.8, .4, .2, 1.),
		Color(.8, .3, 1., 1.)
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
					+ (object.GetPlanet() && !object.GetPlanet()->CanLand() && !object.GetPlanet()->IsWormhole())
					+ 2 * (object.GetPlanet() && object.GetPlanet()->IsWormhole())]);
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
