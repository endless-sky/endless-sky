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
#include "Dialog.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "LineShader.h"
#include "MissionPanel.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "System.h"
#include "Table.h"
#include "UI.h"

#include <algorithm>
#include <cctype>

using namespace std;

namespace {
	const vector<string> RATINGS = {
		"harmless",
		"mostly harmless",
		"not entirely helpless",
		"borderline competent",
		"almost dangerous",
		"moderately intimidating",
		"not to be trifled with",
		"seasoned fighter",
		"respected foe",
		"force to be reckoned with",
		"fearsome scrapper",
		"formidable adversary",
		"dread warrior",
		"veteran battle-lord",
		"terror of the galaxy"
	};
	
	vector<pair<int, string>> Match(const PlayerInfo &player, const string &prefix, const string &suffix)
	{
		vector<pair<int, string>> match;
		
		auto it = player.Conditions().lower_bound(prefix);
		for( ; it != player.Conditions().end(); ++it)
		{
			if(it->first.compare(0, prefix.length(), prefix))
				break;
			if(it->second > 0)
				match.push_back(pair<int, string>(it->second, it->first.substr(prefix.length()) + suffix));
		}
		return match;
	}
	
	void DrawList(vector<pair<int, string>> &list, Table &table, const string &title, int maxCount = 0, bool drawValues = true)
	{
		if(list.empty())
			return;
		
		int otherCount = list.size() - maxCount;
		if(otherCount > 0)
		{
			list[maxCount - 1].second = "(" + to_string(otherCount + 1) + " Others)";
			while(otherCount--)
			{
				list[maxCount - 1].first += list.back().first;
				list.pop_back();
			}
		}
		
		Color dim = *GameData::Colors().Get("medium");
		Color bright = *GameData::Colors().Get("bright");
		table.DrawGap(10);
		table.DrawUnderline(dim);
		table.Draw(title, bright);
		table.Advance();
		table.DrawGap(5);
		
		for(const pair<int, string> &it : list)
		{
			table.Draw(it.second, dim);
			if(drawValues)
				table.Draw(it.first);
			else
				table.Advance();
		}
	}
}



InfoPanel::InfoPanel(PlayerInfo &player)
	: player(player), shipIt(player.Ships().begin()), showShip(false), canEdit(player.GetPlanet())
{
	UpdateInfo();
}



void InfoPanel::Draw() const
{
	DrawBackdrop();
	
	Information interfaceInfo;
	if(showShip)
	{
		interfaceInfo.SetCondition("ship tab");
		if(canEdit && shipIt != player.Ships().begin())
			interfaceInfo.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
		if(player.Ships().size() > 1)
			interfaceInfo.SetCondition("four buttons");
		else
			interfaceInfo.SetCondition("two buttons");
	}
	else
	{
		interfaceInfo.SetCondition("player tab");
		if(canEdit && player.Ships().size() > 1)
		{
			bool allParked = true;
			auto it = player.Ships().begin() + 1;
			for( ; it != player.Ships().end(); ++it)
				allParked &= (*it)->IsParked();
			
			interfaceInfo.SetCondition(allParked ? "show unpark all" : "show park all");
		}
		interfaceInfo.SetCondition("two buttons");
	}
	const Interface *interface = GameData::Interfaces().Get("info panel");
	interface->Draw(interfaceInfo);
	
	zones.clear();
	if(showShip)
		DrawShip();
	else
		DrawInfo();
}



bool InfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'd' || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(showShip && !player.Ships().empty() && (key == 'p' || key == SDLK_LEFT || key == SDLK_UP))
	{
		if(shipIt == player.Ships().begin())
			shipIt = player.Ships().end();
		--shipIt;
		UpdateInfo();
	}
	else if(showShip && !player.Ships().empty() && (key == 'n' || key == SDLK_RIGHT || key == SDLK_DOWN))
	{
		++shipIt;
		if(shipIt == player.Ships().end())
			shipIt = player.Ships().begin();
		UpdateInfo();
	}
	else if(key == 'i' && showShip)
	{
		selected = -1;
		hover = -1;
		showShip = false;
	}
	else if(key == 's')
	{
		showShip = true;
		UpdateInfo();
	}
	else if(showShip && key == 'R')
		GetUI()->Push(new Dialog(this, &InfoPanel::Rename, "Change this ship's name?"));
	else if(canEdit && showShip && key == 'P' && shipIt != player.Ships().begin())
		player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	else if(canEdit && !showShip && key == 'n' && player.Ships().size() > 1)
	{
		bool allParked = true;
		auto it = player.Ships().begin() + 1;
		for( ; it != player.Ships().end(); ++it)
			allParked &= (*it)->IsParked();
		
		it = player.Ships().begin() + 1;
		for( ; it != player.Ships().end(); ++it)
			player.ParkShip(it->get(), !allParked);
	}
	else if(command.Has(Command::INFO | Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else
		return false;
	
	return true;
}



bool InfoPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("info panel");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key)
			return DoKey(key);
	}
	
	if(shipIt == player.Ships().end())
		return true;
	
	dragStart = hoverPoint;
	didDrag = false;
	selected = -1;
	if(showShip)
	{
		if(hover >= 0 && (**shipIt).GetPlanet())
			selected = hover;
	}
	else if(canEdit)
	{
		// Only allow changing your flagship when landed.
		if(hover >= 0)
			selected = hover;
	}
	else if(hover >= 0)
	{
		// If not landed, clicking a ship name takes you straight to its info.
		shipIt = player.Ships().begin() + hover;
		showShip = true;
		UpdateInfo();
	}
	
	return true;
}



bool InfoPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	
	const vector<Armament::Weapon> &weapons = (**shipIt).Weapons();
	hover = -1;
	for(const auto &zone : zones)
		if(zone.Contains(hoverPoint) && (!showShip || selected == -1
				|| weapons[selected].IsTurret() == weapons[zone.Value()].IsTurret()))
			hover = zone.Value();
	
	return true;
}



bool InfoPanel::Drag(int dx, int dy)
{
	Hover(hoverPoint.X() + dx, hoverPoint.Y() + dy);
	if(hoverPoint.Distance(dragStart) > 10.)
		didDrag = true;
	
	return true;
}



bool InfoPanel::Release(int x, int y)
{
	if(selected < 0 || hover < 0)
	{
		selected = -1;
		hover = -1;
		return true;
	}
	
	if(showShip)
	{
		if(hover != selected)
			(**shipIt).GetArmament().Swap(hover, selected);
		selected = -1;
	}
	else if(canEdit)
	{
		if(hover != selected)
			player.ReorderShip(selected, hover);
		else if(!didDrag)
		{
			shipIt = player.Ships().begin() + hover;
			showShip = true;
			UpdateInfo();
		}
		selected = -1;
	}
	return true;
}



void InfoPanel::UpdateInfo()
{
	selected = -1;
	hover = -1;
	if(shipIt == player.Ships().end())
		return;
	
	const Ship &ship = **shipIt;
	info.Update(ship);
	
	outfits.clear();
	for(const auto &it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);
}



void InfoPanel::DrawInfo() const
{
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color elsewhere = *GameData::Colors().Get("dim");
	Color dead(.4, 0., 0., 0.);
	const Font &font = FontSet::Get(14);
	
	// Player info.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(230, Table::RIGHT);
	table.SetUnderline(0, 230);
	table.DrawAt(Point(-490., -265.));
	
	table.Draw("player:", dim);
	table.Draw(player.FirstName() + " " + player.LastName(), bright);
	table.Draw("net worth:", dim);
	table.Draw(Format::Number(player.Accounts().NetWorth()) + " credits", bright);
	
	// Determine the player's combat rating.
	table.DrawGap(10);
	size_t ratingLevel = 0;
	auto it = player.Conditions().find("combat rating");
	if(it != player.Conditions().end() && it->second > 0)
	{
		ratingLevel = log(it->second);
		ratingLevel = min(ratingLevel, RATINGS.size() - 1);
	}
	table.DrawUnderline(dim);
	table.Draw("combat rating:", bright);
	table.Advance();
	table.DrawGap(5);
	table.Draw(RATINGS[ratingLevel], dim);
	table.Draw("(" + to_string(ratingLevel) + ")", dim);
	
	auto salary = Match(player, "salary: ", "");
	sort(salary.begin(), salary.end());
	DrawList(salary, table, "salary:", 4);
	
	auto tribute = Match(player, "tribute: ", "");
	sort(tribute.begin(), tribute.end());
	DrawList(tribute, table, "tribute:", 4);
	
	int maxRows = static_cast<int>(250. - 30. - table.GetPoint().Y()) / 20;
	auto licenses = Match(player, "license: ", " License");
	DrawList(licenses, table, "licenses:", maxRows, false);
	
	// Fleet listing.
	Point pos = Point(-240., -270.);
	font.Draw("ship", pos + Point(0., 0.), bright);
	font.Draw("model", pos + Point(220., 0.), bright);
	font.Draw("system", pos + Point(350., 0.), bright);
	font.Draw("shields", pos + Point(550. - font.Width("shields"), 0.), bright);
	font.Draw("hull", pos + Point(610. - font.Width("hull"), 0.), bright);
	font.Draw("fuel", pos + Point(670. - font.Width("fuel"), 0.), bright);
	font.Draw("crew", pos + Point(730. - font.Width("crew"), 0.), bright);
	FillShader::Fill(pos + Point(365., 15.), Point(730., 1.), dim);
	
	if(!player.Ships().size())
		return;
	int lastIndex = player.Ships().size() - 1;
	
	pos.Y() += 5.;
	int index = 0;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		pos.Y() += 20.;
		if(pos.Y() >= 250.)
			break;
		
		bool isElsewhere = (ship->GetSystem() != player.GetSystem());
		bool isDead = ship->IsDestroyed() || ship->IsDisabled();
		bool isHovered = (index == hover);
		const Color &color = isDead ? dead : isElsewhere ? elsewhere : isHovered ? bright : dim;
		font.Draw(ship->Name(), pos + Point(10. * ship->CanBeCarried(), 0.), color);
		font.Draw(ship->ModelName(), pos + Point(220., 0.), color);
		
		const System *system = ship->GetSystem();
		if(system)
			font.Draw(system->Name(), pos + Point(350., 0.), color);
		
		string shields = to_string(static_cast<int>(100. * max(0., ship->Shields()))) + "%";
		font.Draw(shields, pos + Point(550. - font.Width(shields), 0.), color);
		
		string hull = to_string(static_cast<int>(100. * max(0., ship->Hull()))) + "%";
		font.Draw(hull, pos + Point(610. - font.Width(hull), 0.), color);
		
		string fuel = to_string(static_cast<int>(
			ship->Attributes().Get("fuel capacity") * ship->Fuel()));
		font.Draw(fuel, pos + Point(670. - font.Width(fuel), 0.), color);
		
		string crew = ship->IsParked() ? "Parked" : to_string(ship->Crew());
		font.Draw(crew, pos + Point(730. - font.Width(crew), 0.), color);
		
		if(index < lastIndex || selected < 0)
			zones.emplace_back(pos + Point(365, font.Height() / 2), Point(730, 20), index);
		else
		{
			int height = 270. - pos.Y();
			zones.emplace_back(pos + Point(365, height / 2), Point(730, height), index);
		}
		++index;
	}
	
	// Re-ordering ships in your fleet.
	if(selected >= 0)
	{
		const string &name = player.Ships()[selected]->Name();
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



void InfoPanel::DrawShip() const
{
	if(player.Ships().empty())
		return;
	
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
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
	if(cargo.HasMissionCargo())
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
	SpriteShader::Draw(sprite, shipCenter, scale, 8);
	
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
	if(selected >= 0)
	{
		const Outfit *outfit = ship.Weapons()[selected].GetOutfit();
		string name = outfit ? outfit->Name() : "[empty]";
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



void InfoPanel::DrawWeapon(int index, const Point &pos, const Point &hardpoint) const
{
	if(static_cast<unsigned>(index) >= (**shipIt).Weapons().size())
		return;
	
	const Outfit *outfit = (**shipIt).Weapons()[index].GetOutfit();
	
	const Font &font = FontSet::Get(14);
	Color textColor = *GameData::Colors().Get(index == hover ? "bright" : "medium");
	font.Draw(outfit ? outfit->Name() : "[empty]", pos, textColor);
	
	double high = (index == hover ? .8 : .5);
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
	
	zones.emplace_back(from + Point(120, 0), Point(240, 20), index);
}



void InfoPanel::Rename(const string &name)
{
	if(!name.empty())
	{
		player.RenameShip(shipIt->get(), name);
		UpdateInfo();
	}
}
