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
#include "Preferences.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "System.h"
#include "Table.h"
#include "UI.h"

#include <algorithm>
#include <cmath>

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
		"respected enemy",
		"force to be reckoned with",
		"fearsome scrapper",
		"formidable adversary",
		"dread warrior",
		"veteran battle-lord",
		"legendary foe",
		"war-hungry lunatic",
		"absurdly bloodthirsty",
		"terror of the galaxy",
		"inconceivably destructive",
		"agent of mass extinction",
		"genocidal maniac",
		"destroyer of worlds"
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
				&& (shipIt->get() != player.Flagship() || (*shipIt)->IsParked()))
		{
			if(!(*shipIt)->IsDisabled())
				interfaceInfo.SetCondition("can park");
			interfaceInfo.SetCondition((*shipIt)->IsParked() ? "show unpark" : "show park");
			interfaceInfo.SetCondition("show disown");
		}
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
				{
					interfaceInfo.SetCondition("can park");
					interfaceInfo.SetCondition(allParked ? "show unpark" : "show park");
				}
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
	else if(!showShip && (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN))
	{
		int distance = 24 * ((key == SDLK_PAGEUP) - (key == SDLK_PAGEDOWN));
		scroll = max(0., min(player.Ships().size() - 26., scroll - distance));
	}
	else if(showShip && key == 'R')
		GetUI()->Push(new Dialog(this, &InfoPanel::Rename, "Change this ship's name?"));
	else if(canEdit && showShip && key == 'P')
	{
		if(shipIt->get() != player.Flagship() || (*shipIt)->IsParked())
			player.ParkShip(shipIt->get(), !(*shipIt)->IsParked());
	}
	else if(canEdit && showShip && key == 'D')
	{
		if(shipIt->get() != player.Flagship())
			GetUI()->Push(new Dialog(this, &InfoPanel::Disown, "Are you sure you want to disown \""
				+ shipIt->get()->Name()
				+ "\"? Disowning a ship rather than selling it means you will not get any money for it."));
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
				GetUI()->Push(new Dialog(this, &InfoPanel::DumpCommodities,
					"How many tons of " + Format::LowerCase(selectedCommodity)
						+ " do you want to jettison?", amount));
			}
			else if(plunderAmount > 0 && selectedPlunder->Get("installable") < 0.)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::DumpPlunder,
					"How many tons of " + Format::LowerCase(selectedPlunder->Name())
						+ " do you want to jettison?", plunderAmount));
			}
			else if(plunderAmount == 1)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::Dump,
					"Are you sure you want to jettison a " + selectedPlunder->Name() + "?"));
			}
			else if(plunderAmount > 1)
			{
				GetUI()->Push(new Dialog(this, &InfoPanel::DumpPlunder,
					"How many " + selectedPlunder->PluralName() + " do you want to jettison?",
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
	else if(canEdit && !showShip && key == 'A' && player.Ships().size() > 1)
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



bool InfoPanel::Click(int x, int y, int clicks)
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
	else if(canEdit && (shift || control || clicks < 2))
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



bool InfoPanel::Hover(int x, int y)
{
	Point point(x, y);
	info.Hover(point);
	return Hover(point);
}



bool InfoPanel::Drag(double dx, double dy)
{
	Hover(hoverPoint + Point(dx, dy));
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
		scroll = max(0., min(player.Ships().size() - 26., scroll - dy * .1 * Preferences::ScrollSpeed()));
	return true;
}



void InfoPanel::UpdateInfo()
{
	selected = -1;
	hover = -1;
	if(shipIt == player.Ships().end())
		return;
	
	const Ship &ship = **shipIt;
	info.Update(ship, player.FleetDepreciation(), player.GetDate().DaysSinceEpoch());
	if(player.Flagship() && ship.GetSystem() == player.GetSystem() && &ship != player.Flagship())
		player.Flagship()->SetTargetShip(*shipIt);
	
	outfits.clear();
	for(const auto &it : ship.Outfits())
		outfits[it.first->Category()].push_back(it.first);
}



void InfoPanel::DrawInfo()
{
	const Interface *interface = GameData::Interfaces().Get("info panel");
	DrawPlayer(interface->GetBox("player"));
	DrawFleet(interface->GetBox("fleet"));
}



void InfoPanel::DrawPlayer(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 250.)
		return;
	
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(230, Table::RIGHT);
	table.SetUnderline(0, 230);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	
	// Header row.
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
}



void InfoPanel::DrawFleet(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 750.)
		return;
	
	// Colors to draw with.
	Color back = *GameData::Colors().Get("faint");
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	Color elsewhere = *GameData::Colors().Get("dim");
	Color dead(.4, 0., 0., 0.);
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(220, Table::LEFT);
	table.AddColumn(350, Table::LEFT);
	table.AddColumn(550, Table::RIGHT);
	table.AddColumn(610, Table::RIGHT);
	table.AddColumn(670, Table::RIGHT);
	table.AddColumn(730, Table::RIGHT);
	table.SetUnderline(0, 730);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	
	// Header row.
	table.DrawUnderline(dim);
	table.SetColor(bright);
	table.Draw("ship");
	table.Draw("model");
	table.Draw("system");
	table.Draw("shields");
	table.Draw("hull");
	table.Draw("fuel");
	table.Draw("crew");
	table.DrawGap(5);
	
	// Loop through all the player's ships.
	int index = scroll;
	auto sit = player.Ships().begin() + scroll;
	for( ; sit < player.Ships().end(); ++sit)
	{
		// Bail out if we've used out the whole drawing area.
		if(!bounds.Contains(table.GetRowBounds()))
			break;
		
		// Check if this row is selected.
		if(allSelected.count(index))
			table.DrawHighlight(back);
		
		const Ship &ship = **sit;
		bool isElsewhere = (ship.GetSystem() != player.GetSystem());
		isElsewhere |= (ship.CanBeCarried() && player.GetPlanet());
		bool isDead = ship.IsDestroyed() || ship.IsDisabled();
		bool isHovered = (index == hover);
		table.SetColor(isDead ? dead : isElsewhere ? elsewhere : isHovered ? bright : dim);
		
		// Store this row's position, to handle hovering.
		zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), index);
		
		// Indent the ship name if it is a fighter or drone.
		table.Draw(ship.CanBeCarried() ? "    " + ship.Name() : ship.Name());
		table.Draw(ship.ModelName());
		
		const System *system = ship.GetSystem();
		table.Draw(system ? system->Name() : "");
		
		string shields = to_string(static_cast<int>(100. * max(0., ship.Shields()))) + "%";
		table.Draw(shields);
		
		string hull = to_string(static_cast<int>(100. * max(0., ship.Hull()))) + "%";
		table.Draw(hull);
		
		string fuel = to_string(static_cast<int>(
			ship.Attributes().Get("fuel capacity") * ship.Fuel()));
		table.Draw(fuel);
		
		// If this isn't the flagship, we'll remember how many crew it has, but
		// only the minimum number of crew need to be paid for.
		int crewCount = ship.Crew();
		if(&ship != player.Flagship())
			crewCount = min(crewCount, ship.RequiredCrew());
		string crew = (ship.IsParked() ? "Parked" : to_string(crewCount));
		table.Draw(crew);
		
		++index;
	}
	
	// Re-ordering ships in your fleet.
	if(selected >= 0)
	{
		const Font &font = FontSet::Get(14);
		const string &name = player.Ships()[selected]->Name();
		Point pos(hoverPoint.X() - .5 * font.Width(name), hoverPoint.Y());
		font.Draw(name, pos + Point(1., 1.), Color(0., 1.));
		font.Draw(name, pos, bright);
	}
}



void InfoPanel::DrawShip()
{
	if(player.Ships().empty() || shipIt == player.Ships().end())
		return;
	
	const Interface *interface = GameData::Interfaces().Get("info panel");
	DrawShipStats(interface->GetBox("stats"));
	DrawOutfits(interface->GetBox("outfits"));
	DrawWeapons(interface->GetBox("weapons"));
	DrawCargo(interface->GetBox("cargo"));
	
	info.DrawTooltips();
}



void InfoPanel::DrawShipStats(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 250.)
		return;
	
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship &ship = **shipIt;
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(230, Table::RIGHT);
	table.SetUnderline(0, 230);
	table.DrawAt(bounds.TopLeft() + Point(10., 8.));
	
	// Draw the ship information.
	table.Draw("ship:", dim);
	table.Draw(ship.Name(), bright);
	
	table.Draw("model:", dim);
	table.Draw(ship.ModelName(), bright);
	
	info.DrawAttributes(table.GetRowBounds().TopLeft() - Point(10., 10.));
}



void InfoPanel::DrawOutfits(const Rectangle &bounds)
{
	// Check that the specified area is big enough.
	if(bounds.Width() < 250.)
		return;
	
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Ship &ship = **shipIt;
	
	// Table attributes.
	Table table;
	table.AddColumn(0, Table::LEFT);
	table.AddColumn(230, Table::RIGHT);
	table.SetUnderline(0, 230);
	Point start = bounds.TopLeft() + Point(10., 8.);
	table.DrawAt(start);
	
	// Draw the outfits in the same order used in the outfitter.
	for(const string &category : Outfit::CATEGORIES)
	{
		auto it = outfits.find(category);
		if(it == outfits.end())
			continue;
		
		// Skip to the next column if there is not space for this category label
		// plus at least one outfit.
		if(table.GetRowBounds().Bottom() + 40. > bounds.Bottom())
		{
			start += Point(250., 0.);
			if(start.X() + 230. > bounds.Right())
				break;
			table.DrawAt(start);
		}
		
		// Draw the category label.
		table.Draw(category, bright);
		table.Advance();
		for(const Outfit *outfit : it->second)
		{
			// Check if we've gone below the bottom of the bounds.
			if(table.GetRowBounds().Bottom() > bounds.Bottom())
			{
				start += Point(250., 0.);
				if(start.X() + 230. > bounds.Right())
					break;
				table.DrawAt(start);
				table.Draw(category, bright);
				table.Advance();
			}
			
			// Draw the outfit name and count.
			table.Draw(outfit->Name(), dim);
			string number = to_string(ship.OutfitCount(outfit));
			table.Draw(number, bright);
		}
		// Add an extra gap in between categories.
		table.DrawGap(10.);
	}
}



void InfoPanel::DrawWeapons(const Rectangle &bounds)
{
	// Colors to draw with.
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Font &font = FontSet::Get(14);
	const Ship &ship = **shipIt;
	
	// Figure out how much to scale the sprite by.
	const Sprite *sprite = ship.GetSprite();
	double scale = 0.;
	if(sprite)
		scale = min(240. / sprite->Width(), 240. / sprite->Height());
	
	// Figure out the left- and right-most hardpoints on the ship. If they are
	// too far apart, the scale may need to be reduced.
	// Also figure out how many weapons of each type are on each side.
	double maxX = 0.;
	int count[2][2] = {{0, 0}, {0, 0}};
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		// Multiply hardpoint X by 2 to convert to sprite pixels.
		maxX = max(maxX, fabs(2. * hardpoint.GetPoint().X()));
		++count[hardpoint.GetPoint().X() >= 0.][hardpoint.IsTurret()];
	}
	// If necessary, shrink the sprite to keep the hardpoints inside the labels.
	// The width of this UI block will be 2 * (LABEL_WIDTH + HARDPOINT_DX).
	static const double LABEL_WIDTH = 150.;
	static const double LABEL_DX = 95.;
	static const double LABEL_PAD = 5.;
	if(maxX > (LABEL_DX - LABEL_PAD))
		scale = min(scale, (LABEL_DX - LABEL_PAD) / (2. * maxX));
	
	// Draw the ship, using the black silhouette swizzle.
	SpriteShader::Draw(sprite, bounds.Center(), scale, 8);
	
	// Figure out how tall each part of the weapon listing will be.
	int gunRows = max(count[0][0], count[1][0]);
	int turretRows = max(count[0][1], count[1][1]);
	// If there are both guns and turrets, add a gap of ten pixels.
	double height = 20. * (gunRows + turretRows) + 10. * (gunRows && turretRows);
	
	double gunY = bounds.Top() + .5 * (bounds.Height() - height);
	double turretY = gunY + 20. * gunRows + 10. * (gunRows != 0);
	double nextY[2][2] = {
		{gunY + 20. * (gunRows - count[0][0]), turretY + 20. * (turretRows - count[0][1])},
		{gunY + 20. * (gunRows - count[1][0]), turretY + 20. * (turretRows - count[1][1])}};
	
	int index = 0;
	const double centerX = bounds.Center().X();
	const double labelCenter[2] = {-.5 * LABEL_WIDTH - LABEL_DX, LABEL_DX + .5 * LABEL_WIDTH};
	const double fromX[2] = {-LABEL_DX + LABEL_PAD, LABEL_DX - LABEL_PAD};
	static const double LINE_HEIGHT = 20.;
	static const double TEXT_OFF = .5 * (LINE_HEIGHT - font.Height());
	static const Point LINE_SIZE(LABEL_WIDTH, LINE_HEIGHT);
	Point topFrom;
	Point topTo;
	Color topColor;
	bool hasTop = false;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		string name = "[empty]";
		if(hardpoint.GetOutfit())
			name = font.Truncate(hardpoint.GetOutfit()->Name(), 150);
		
		bool isRight = (hardpoint.GetPoint().X() >= 0.);
		bool isTurret = hardpoint.IsTurret();
		
		double &y = nextY[isRight][isTurret];
		double x = centerX + (isRight ? LABEL_DX : (-LABEL_DX - font.Width(name)));
		bool isHover = (index == hover);
		font.Draw(name, Point(x, y + TEXT_OFF), isHover ? bright : dim);
		Point zoneCenter(labelCenter[isRight], y + .5 * LINE_HEIGHT);
		zones.emplace_back(zoneCenter, LINE_SIZE, index);
		
		// Determine what color to use for the line.
		double high = (index == hover ? .8 : .5);
		Color color(high, .75 * high, 0., 1.);
		if(isTurret)
			color = Color(0., .75 * high, high, 1.);
		
		// Draw the line.
		Point from(fromX[isRight], zoneCenter.Y());
		Point to = bounds.Center() + (2. * scale) * hardpoint.GetPoint();
		DrawLine(from, to, color);
		if(isHover)
		{
			topFrom = from;
			topTo = to;
			topColor = color;
			hasTop = true;
		}
		
		y += LINE_HEIGHT;
		++index;
	}
	// Make sure the line for whatever hardpoint we're hovering is always on top.
	if(hasTop)
		DrawLine(topFrom, topTo, topColor);
	
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



void InfoPanel::DrawCargo(const Rectangle &bounds)
{
	Color dim = *GameData::Colors().Get("medium");
	Color bright = *GameData::Colors().Get("bright");
	const Font &font = FontSet::Get(14);
	const Ship &ship = **shipIt;

	// Cargo list.
	const CargoHold &cargo = (player.Cargo().Used() ? player.Cargo() : ship.Cargo());
	Point pos = Point(260., -280.);
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
			bool isSingular = (it.second == 1 || it.first->Get("installable") < 0.);
			font.Draw(isSingular ? it.first->Name() : it.first->PluralName(), pos, dim);
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
}



void InfoPanel::DrawLine(const Point &from, const Point &to, const Color &color) const
{
	Color black(0., 1.);
	Point mid(to.X(), from.Y());
	
	LineShader::Draw(from, mid, 3.5, black);
	LineShader::Draw(mid, to, 3.5, black);
	LineShader::Draw(from, mid, 1.5, color);
	LineShader::Draw(mid, to, 1.5, color);
}



bool InfoPanel::Hover(const Point &point)
{
	if(shipIt == player.Ships().end())
		return true;

	hoverPoint = point;
	
	const vector<Hardpoint> &weapons = (**shipIt).Weapons();
	hover = -1;
	for(const auto &zone : zones)
		if(zone.Contains(hoverPoint) && (!showShip || selected == -1
				|| weapons[selected].IsTurret() == weapons[zone.Value()].IsTurret()))
			hover = zone.Value();
	
	return true;
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
	
	info.Update(**shipIt, player.FleetDepreciation(), player.GetDate().DaysSinceEpoch());
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
		info.Update(**shipIt, player.FleetDepreciation(), player.GetDate().DaysSinceEpoch());
		
		if(loss)
			Messages::Add("You jettisoned " + Format::Number(loss) + " credits worth of cargo.");
	}
}



void InfoPanel::DumpCommodities(int count)
{
	int64_t loss = 0;
	count = min(count, (*shipIt)->Cargo().Get(selectedCommodity));
	if(count > 0)
	{
		int64_t basis = player.GetBasis(selectedCommodity, count);
		loss += basis;
		player.AdjustBasis(selectedCommodity, -basis);
		(*shipIt)->Jettison(selectedCommodity, count);
		info.Update(**shipIt, player.FleetDepreciation(), player.GetDate().DaysSinceEpoch());
		
		if(loss)
			Messages::Add("You jettisoned " + Format::Number(loss) + " credits worth of cargo.");
	}
}



void InfoPanel::Disown()
{
	// Make sure a ship really is selected.
	if(shipIt == player.Ships().end() || !shipIt->get())
		return;
	
	const Ship *ship = shipIt->get();
	if(shipIt != player.Ships().begin())
		--shipIt;
	else
		++shipIt;
	
	player.DisownShip(ship);
	allSelected.clear();
	showShip = false;
}
