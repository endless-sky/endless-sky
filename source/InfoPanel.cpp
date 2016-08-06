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
#include "Messages.h"
#include "MissionPanel.h"
#include "PlayerInfo.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "System.h"
#include "Table.h"
#include "UI.h"

#include <algorithm>

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
		
		if(otherCount > 0 && maxCount > 0)
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



InfoPanel::InfoPanel(PlayerInfo &player, bool showFlagship)
	: player(player), shipIt(player.Ships().begin()), showShip(showFlagship), canEdit(player.GetPlanet())
{
	SetInterruptible(false);
	
	if(showFlagship)
		while(shipIt != player.Ships().end() && shipIt->get() != player.Flagship())
			++shipIt;
	
	UpdateInfo();
}



void InfoPanel::Draw()
{
	DrawBackdrop();
	
	Information interfaceInfo;
	if(showShip)
	{
		interfaceInfo.SetCondition("ship tab");
		if(canEdit && (shipIt != player.Ships().end())
					&& ((shipIt->get() != player.Flagship() && !(*shipIt)->IsDisabled()) || (*shipIt)->IsParked()))
			interfaceInfo.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
		else if(!canEdit)
		{
			interfaceInfo.SetCondition("show dump");
			if(CanDump())
				interfaceInfo.SetCondition("enable dump");
		}
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
			bool hasOtherShips = false;
			const Ship *flagship = player.Flagship();
			for(const auto &it : player.Ships())
				if(!it->IsDisabled() && it.get() != flagship)
				{
					allParked &= it->IsParked();
					hasOtherShips = true;
				}
			if(hasOtherShips)
				interfaceInfo.SetCondition(allParked ? "show unpark all" : "show park all");
			
			if(!allSelected.empty())
			{
				bool parkable = false;
				allParked = true;
				for(int i : allSelected)
				{
					const Ship &ship = *player.Ships()[i];
					if(!ship.IsDisabled() && &ship != flagship)
					{
						allParked &= ship.IsParked();
						parkable = true;
					}
				}
				if(parkable)
					interfaceInfo.SetCondition(allParked ? "show unpark" : "show park");
			}
		}
		interfaceInfo.SetCondition("two buttons");
	}
	const Interface *interface = GameData::Interfaces().Get("info panel");
	interface->Draw(interfaceInfo, this);
	
	zones.clear();
	commodityZones.clear();
	if(showShip)
		DrawShip();
	else
		DrawInfo();
}



bool InfoPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if(key == 'd' || key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
		Scroll(0, 6 * ((key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN)));
	else if(showShip && key == 'R')
		GetUI()->Push(new Dialog(this, &InfoPanel::Rename, "Change this ship's name?"));
	else if(canEdit && showShip && key == 'P')
	{
		if(shipIt->get() != player.Flagship() || (*shipIt)->IsParked())
			player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	}
	else if(canEdit && key == 'P')
	{
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(int i : allSelected)
		{
			const Ship &ship = *player.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				allParked &= ship.IsParked();
		}
		
		for(int i : allSelected)
		{
			const Ship &ship = *player.Ships()[i];
			if(!ship.IsDisabled() && &ship != flagship)
				player.ParkShip(&ship, !allParked);
		}
	}
	else if((key == 'P' || key == 'c') && showShip && !canEdit)
	{
		if(CanDump())
		{
			int commodities = (*shipIt)->Cargo().CommoditiesSize();
			int amount = (*shipIt)->Cargo().Get(selectedCommodity);
			int plunderAmount = (*shipIt)->Cargo().Get(selectedPlunder);
			if(amount)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::Dump,
					"Are you sure you want to jettison "
						+ (amount == 1 ? "a ton" : Format::Number(amount) + " tons")
						+ " of " + Format::LowerCase(selectedCommodity) + " cargo?"));
			}
			else if(plunderAmount > 0 && selectedPlunder->Get("installable") < 0.)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::DumpPlunder,
					"How many tons of " + Format::LowerCase(selectedPlunder->Name()) + " do you want to jettison?",
					plunderAmount));
			}
			else if(plunderAmount == 1)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::Dump,
					"Are you sure you want to jettison a " + selectedPlunder->Name() + "?"));
			}
			else if(plunderAmount > 1)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::DumpPlunder,
					"How many of the " + selectedPlunder->Name() + " outfits to you want to jettison?",
					plunderAmount));
			}
			else if(commodities)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::Dump,
					"Are you sure you want to jettison all this ship's regular cargo?"));
			}
			else
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::Dump,
					"Are you sure you want to jettison all this ship's spare outfit cargo?"));
			}
		}
	}
	else if(canEdit && !showShip && key == 'n' && player.Ships().size() > 1)
	{
		bool allParked = true;
		const Ship *flagship = player.Flagship();
		for(const auto &it : player.Ships())
			if(!it->IsDisabled() && it.get() != flagship)
				allParked &= it->IsParked();
		
		for(const auto &it : player.Ships())
			if(!it->IsDisabled() && (allParked || it.get() != flagship))
				player.ParkShip(it.get(), !allParked);
	}
	else if(command.Has(Command::INFO | Command::MAP) || key == 'm')
		GetUI()->Push(new MissionPanel(player));
	else
		return false;
	
	return true;
}



bool InfoPanel::Click(int x, int y)
{
	if(shipIt == player.Ships().end())
		return true;
	
	dragStart = hoverPoint;
	didDrag = false;
	selected = -1;
	bool shift = (SDL_GetModState() & KMOD_SHIFT);
	bool control = (SDL_GetModState() & (KMOD_CTRL | KMOD_GUI));
	if(showShip)
	{
		if(hover >= 0 && (**shipIt).GetSystem() == player.GetSystem() && !(**shipIt).IsDisabled())
			selected = hover;
	}
	else if(canEdit && (shift || control || hover != previousSelected))
	{
		// Only allow changing your flagship when landed.
		if(hover >= 0)
		{
			if(shift)
			{
				// Select all the ships between the previous selection and this one.
				for(int i = max(0, min(previousSelected, hover)); i < max(previousSelected, hover); ++i)
					allSelected.insert(i);
			}
			else if(!control)
			{
				allSelected.clear();
				selected = hover;
			}
			
			if(control && allSelected.count(hover))
				allSelected.erase(hover);
			else
			{
				allSelected.insert(hover);
				previousSelected = hover;
			}
		}
	}
	else if(hover >= 0)
	{
		// If not landed, clicking a ship name takes you straight to its info.
		shipIt = player.Ships().begin() + hover;
		showShip = true;
		UpdateInfo();
	}
	selectedCommodity.clear();
	selectedPlunder = nullptr;
	Point point(x, y);
	for(const auto &zone : commodityZones)
		if(zone.Contains(point))
			selectedCommodity = zone.Value();
	for(const auto &zone : plunderZones)
		if(zone.Contains(point))
			selectedPlunder = zone.Value();
	
	return true;
}



bool InfoPanel::Hover(double x, double y)
{
	if(shipIt == player.Ships().end())
		return true;

	hoverPoint = Point(x, y);
	
	const vector<Hardpoint> &weapons = (**shipIt).Weapons();
	hover = -1;
	for(const auto &zone : zones)
		if(zone.Contains(hoverPoint) && (!showShip || selected == -1
				|| weapons[selected].IsTurret() == weapons[zone.Value()].IsTurret()))
			hover = zone.Value();
	
	return true;
}



bool InfoPanel::Hover(int x, int y)
{
	info.Hover(Point(x, y));
	return Hover(static_cast<double>(x), static_cast<double>(y));
}



bool InfoPanel::Drag(double dx, double dy)
{
	Hover(hoverPoint.X() + dx, hoverPoint.Y() + dy);
	if(hoverPoint.Distance(dragStart) > 10.)
		didDrag = true;
	
	return true;
}



bool InfoPanel::Release(int x, int y)
{
	if(selected >= 0 && hover >= 0)
	{
		if(showShip)
		{
			if(hover != selected)
				(**shipIt).GetArmament().Swap(hover, selected);
		}
		else if(canEdit)
		{
			if(hover != selected)
				player.ReorderShip(selected, hover);
		}
	}
	selected = -1;
	return true;
}



bool InfoPanel::Scroll(double dx, double dy)
{
	if(!showShip)
		scroll = max(0., min(player.Ships().size() - 26., scroll - 4. * dy));
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
	Color back = *GameData::Colors().Get("faint");
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
	table.DrawAt(Point(-490., -282.));
	
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
	Point pos = Point(-240., -280.);
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
	const Ship *flagship = player.Flagship();
	
	Point zoneSize(730., 20.);
	Point zoneCenter(.5 * zoneSize.X(), .5 * font.Height());
	
	pos.Y() += 5.;
	int index = scroll;
	auto sit = player.Ships().begin() + scroll;
	for( ; sit < player.Ships().end(); ++sit)
	{
		const shared_ptr<Ship> &ship = *sit;
		pos.Y() += 20.;
		if(pos.Y() >= 260.)
			break;
		
		if(allSelected.count(index))
			FillShader::Fill(pos + zoneCenter, zoneSize + Point(10., 0.), back);
		
		bool isElsewhere = (ship->GetSystem() != player.GetSystem());
		isElsewhere |= (ship->CanBeCarried() && player.GetPlanet());
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
		
		string crew = ship->IsParked() ? "Parked" :
			to_string(ship.get() == flagship ? ship->Crew() : ship->RequiredCrew());
		font.Draw(crew, pos + Point(730. - font.Width(crew), 0.), color);
		
		if(index < lastIndex || selected < 0)
			zones.emplace_back(pos + zoneCenter, zoneSize, index);
		else
		{
			Point size(zoneSize.X(), 280. - pos.Y());
			zones.emplace_back(pos + .5 * size, size, index);
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
	font.Draw("ship:", Point(-490., -280.), dim);
	Point shipNamePos(-260 - font.Width(ship.Name()), -280.);
	font.Draw(ship.Name(), shipNamePos, bright);
	font.Draw("model:", Point(-490., -260.), dim);
	Point modelNamePos(-260 - font.Width(ship.ModelName()), -260.);
	font.Draw(ship.ModelName(), modelNamePos, bright);
	info.DrawAttributes(Point(-500., -250.));
	
	// Outfits list.
	Point pos(-240., -280.);
	for(const auto &it : outfits)
	{
		int height = 20 * (it.second.size() + 1);
		if(pos.X() == -240. && pos.Y() + height > 30.)
			pos = Point(pos.X() + 250., -280.);
		
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
	pos = Point(260., -280.);
	static const Point size(230., 20.);
	Color backColor = *GameData::Colors().Get("faint");
	if(cargo.CommoditiesSize() || cargo.HasOutfits() || cargo.MissionCargoSize())
	{
		font.Draw("Cargo", pos, bright);
		pos.Y() += 20.;
	}
	if(cargo.CommoditiesSize())
	{
		for(const auto &it : cargo.Commodities())
		{
			if(!it.second)
				continue;
			
			Point center = pos + .5 * size - Point(0., (20 - font.Height()) * .5);
			commodityZones.emplace_back(center, size, it.first);
			if(it.first == selectedCommodity)
				FillShader::Fill(center, size + Point(10., 0.), backColor);
			
			string number = to_string(it.second);
			Point numberPos(pos.X() + size.X() - font.Width(number), pos.Y());
			font.Draw(it.first, pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += size.Y();
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 230.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.HasOutfits() && pos.Y() < 230.)
	{
		for(const auto &it : cargo.Outfits())
		{
			if(!it.second)
				continue;
			
			Point center = pos + .5 * size - Point(0., (20 - font.Height()) * .5);
			plunderZones.emplace_back(center, size, it.first);
			if(it.first == selectedPlunder)
				FillShader::Fill(center, size + Point(10., 0.), backColor);
			
			string number = to_string(it.second);
			Point numberPos(pos.X() + size.X() - font.Width(number), pos.Y());
			font.Draw(it.first->Name(), pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += size.Y();
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 230.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.HasMissionCargo() && pos.Y() < 230.)
	{
		for(const auto &it : cargo.MissionCargo())
		{
			// Capitalize the name of the cargo.
			string name = Format::Capitalize(it.first->Cargo());
			
			string number = to_string(it.second);
			Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
			font.Draw(name, pos, dim);
			font.Draw(number, numberPos, bright);
			pos.Y() += 20.;
			
			// Truncate the list if there is not enough space.
			if(pos.Y() >= 230.)
				break;
		}
		pos.Y() += 10.;
	}
	if(cargo.Passengers())
	{
		pos = Point(pos.X(), 260.);
		string number = to_string(cargo.Passengers());
		Point numberPos(pos.X() + 230. - font.Width(number), pos.Y());
		font.Draw("passengers:", pos, dim);
		font.Draw(number, numberPos, bright);
	}
	
	// Weapon positions.
	const Sprite *sprite = ship.GetSprite();
	double scale = min(240. / sprite->Width(), 240. / sprite->Height());
	Point shipCenter(-125., 155.);
	SpriteShader::Draw(sprite, shipCenter, scale, 8);
	
	Color black(0., 1.);
	pos = Point(10., 260.);
	Point hoverPos;
	for(int isTurret = true; isTurret >= 0; --isTurret)
	{
		for(unsigned i = 0; i < ship.Weapons().size(); ++i)
		{
			const Hardpoint &weapon = ship.Weapons()[i];
			if(weapon.IsTurret() == isTurret)
			{
				if(static_cast<int>(i) == hover)
					hoverPos = pos;
				else
					DrawWeapon(i, pos, shipCenter + (2. * scale) * weapon.GetPoint());
				pos.Y() -= 20.;
			}
		}
		if(pos.Y() != 260.)
			pos.Y() -= 10.;
	}
	if(hover >= 0 && hover <= static_cast<int>(ship.Weapons().size()))
		DrawWeapon(hover, hoverPos, shipCenter + (2. * scale) * ship.Weapons()[hover].GetPoint());
	
	info.DrawTooltips();
	
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



bool InfoPanel::CanDump() const
{
	if(canEdit || !showShip || shipIt == player.Ships().end())
		return false;
	
	CargoHold &cargo = (*shipIt)->Cargo();
	return (selectedPlunder && cargo.Get(selectedPlunder) > 0) || cargo.CommoditiesSize() || cargo.OutfitsSize();
}



void InfoPanel::Dump()
{
	if(!CanDump())
		return;
	
	CargoHold &cargo = (*shipIt)->Cargo();
	int commodities = (*shipIt)->Cargo().CommoditiesSize();
	int amount = cargo.Get(selectedCommodity);
	int plunderAmount = cargo.Get(selectedPlunder);
	int64_t loss = 0;
	if(amount)
	{
		int64_t basis = player.GetBasis(selectedCommodity, amount);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, amount);
	}
	else if(plunderAmount > 0)
	{
		loss += plunderAmount * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, plunderAmount);
	}
	else if(commodities)
	{
		for(const auto &it : cargo.Commodities())
		{
			int64_t basis = player.GetBasis(it.first, it.second);
			loss += basis;
			player.AdjustBasis(it.first, -basis);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	else
	{
		for(const auto &it : cargo.Outfits())
		{
			loss += it.first->Cost() * max(0, it.second);
			(*shipIt)->Jettison(it.first, it.second);
		}
	}
	selectedCommodity.clear();
	selectedPlunder = nullptr;
	
	info.Update(**shipIt);
	if(loss)
		Messages::Add("You jettisoned " + Format::Number(loss) + " credits worth of cargo.");
}



void InfoPanel::DumpPlunder(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedPlunder));
	if(count > 0)
	{
		loss += count * selectedPlunder->Cost();
		(*shipIt)->Jettison(selectedPlunder, count);
		info.Update(**shipIt);
		
		if(loss)
			Messages::Add("You jettisoned " + Format::Number(loss) + " credits worth of cargo.");
	}
}
