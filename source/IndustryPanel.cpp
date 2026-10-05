/* IndustryPanel.cpp
Copyright (c) 2026 by Endless Sky Expanded contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "IndustryPanel.h"

#include "text/Alignment.h"
#include "CargoHold.h"
#include "Color.h"
#include "Command.h"
#include "Facility.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Industry.h"
#include "Interface.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Port.h"
#include "Rectangle.h"
#include "Screen.h"
#include "System.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <functional>

using namespace std;

namespace {
	const double PAD = 10.;
	const double LINE = 18.;
	const double ROW = 20.;
	const double LIST_WIDTH = 170.;
	const Point BUTTON_SIZE(68., 22.);
	const double BUTTON_GAP = 6.;

	// Join a list of phrases with commas.
	string Join(const vector<string> &parts)
	{
		string result;
		for(const string &part : parts)
			result += (result.empty() ? "" : ", ") + part;
		return result;
	}

	// For example, "2 Metal, 1 Plastic".
	string AmountsString(const Facility::Amounts &amounts, int count)
	{
		vector<string> parts;
		for(const auto &[commodity, amount] : amounts)
			parts.push_back(to_string(amount * count) + " " + commodity);
		return Join(parts);
	}

	string StatusString(Industry::Status status)
	{
		switch(status)
		{
			case Industry::Status::STARTING:
				return "Starting up";
			case Industry::Status::RUNNING:
				return "Running";
			case Industry::Status::NO_INPUTS:
				return "Idle: needs inputs";
			case Industry::Status::STORAGE_FULL:
				return "Idle: storage full";
			case Industry::Status::NO_CREDITS:
				return "Idle: can't pay upkeep";
		}
		return "";
	}

	const Rectangle ContentBox()
	{
		return GameData::Interfaces().Get(Screen::Width() < 1280 ? "planet (small screen)" : "planet")
			->GetBox("content");
	}

	int VisibleRows(const Rectangle &box, double top)
	{
		return max(1, static_cast<int>((box.Bottom() - 2. * LINE - top) / ROW));
	}
}



IndustryPanel::IndustryPanel(PlayerInfo &player, const Planet &planet)
	: player(player), planet(planet)
{
	// Let the planet panel underneath keep handling its own buttons and keys.
	SetTrapAllEvents(false);
	SetInterruptible(false);
	showOverview = Facilities(player, planet).empty();
}



bool IndustryPanel::IsAvailable(const PlayerInfo &player, const Planet &planet)
{
	// Uninhabited worlds have no services to deny, so outposts can be built there.
	if(planet.IsInhabited() && !planet.CanUseServices())
		return false;
	return !player.GetIndustry().Holdings().empty() || !Facilities(player, planet).empty();
}



void IndustryPanel::Draw()
{
	ClearZones();
	if(showOverview || Facilities(player, planet).empty())
		DrawOverview();
	else
		DrawPlanetView();

	// The bottom line shows the result of the last action, or a reminder of the keys.
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Point bottom(box.Left() + PAD, box.Bottom() - LINE);
	if(!status.empty())
		font.Draw(status, bottom, *GameData::Colors().Get("bright"));
	else if(showOverview || Facilities(player, planet).empty())
		font.Draw("Up/Down: scroll    Tab: this planet", bottom, *GameData::Colors().Get("dim"));
	else
		font.Draw("E: build    U: supply    C: collect    A: auto-sell    Tab: all holdings",
			bottom, *GameData::Colors().Get("dim"));
}



bool IndustryPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	const bool hasLocal = !Facilities(player, planet).empty();
	if(key == SDLK_TAB)
	{
		showOverview = !showOverview || !hasLocal;
		scroll = 0;
		status.clear();
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		const int step = (key == SDLK_UP ? -1 : 1);
		if(showOverview || !hasLocal)
			scroll = max(0, scroll + step);
		else
			selectedRow = max(0, selectedRow + step);
	}
	else if(showOverview || !hasLocal)
		return false;
	else if(key == 'e')
		Build();
	else if(key == 'u')
		Supply();
	else if(key == 'c')
		Collect();
	else if(key == 'a')
		ToggleAutoSell();
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER)
	{
		const Facility *facility = Selected();
		if(facility && player.GetIndustry().Find(*facility, planet.TrueName()))
			Collect();
		else
			Build();
	}
	else
		return false;

	return true;
}



bool IndustryPanel::Scroll(double dx, double dy)
{
	if(dy)
		scroll = max(0, scroll + (dy > 0. ? -1 : 1));
	return true;
}



vector<const Facility *> IndustryPanel::Facilities(const PlayerInfo &player, const Planet &planet)
{
	vector<const Facility *> result;
	for(const auto &it : GameData::Facilities())
	{
		const Facility &facility = it.second;
		if(!facility.IsDefined())
			continue;
		// Some facilities have to be unlocked before they can be built.
		const string &requirement = facility.Requirement();
		const bool isUnlocked = requirement.empty() || player.Conditions().Get(requirement) > 0;
		if((isUnlocked && facility.CanBuildOn(planet.TrueName(), planet.Attributes(), planet.IsInhabited()))
				|| player.GetIndustry().Find(facility, planet.TrueName()))
			result.push_back(&facility);
	}
	return result;
}



const Facility *IndustryPanel::Selected() const
{
	const vector<const Facility *> facilities = Facilities(player, planet);
	if(facilities.empty())
		return nullptr;
	return facilities[clamp(selectedRow, 0, static_cast<int>(facilities.size()) - 1)];
}



void IndustryPanel::DrawPlanetView()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &faint = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const vector<const Facility *> facilities = Facilities(player, planet);
	selectedRow = clamp(selectedRow, 0, static_cast<int>(facilities.size()) - 1);
	const Industry &industry = player.GetIndustry();

	font.Draw("Industry on " + planet.DisplayName(), box.TopLeft() + Point(PAD, 0.), bright);

	// List of facilities on the left. Scroll it to keep the selection visible.
	const double listTop = box.Top() + LINE + 6.;
	const int rows = VisibleRows(box, listTop);
	scroll = clamp(scroll, max(0, selectedRow - rows + 1), selectedRow);
	for(int i = scroll; i < static_cast<int>(facilities.size()) && i < scroll + rows; ++i)
	{
		const Facility &facility = *facilities[i];
		const Industry::Holding *holding = industry.Find(facility, planet.TrueName());
		const Point corner(box.Left(), listTop + (i - scroll) * ROW);
		const Rectangle row = Rectangle::FromCorner(corner, Point(LIST_WIDTH, ROW));
		if(i == selectedRow)
			FillShader::Fill(row, faint);
		string label = facility.TrueName();
		if(holding)
			label += " x" + to_string(holding->count);
		font.Draw(label, corner + Point(PAD, .5 * (ROW - font.Height())), i == selectedRow ? bright : medium);
		AddZone(row, [this, i]() { selectedRow = i; status.clear(); });
	}

	// Details of the selected facility on the right.
	const Facility &facility = *facilities[selectedRow];
	const Industry::Holding *holding = industry.Find(facility, planet.TrueName());
	const int count = holding ? holding->count : 1;
	const double left = box.Left() + LIST_WIDTH + PAD;
	const double width = box.Right() - PAD - left;
	Point pos(left, listTop);
	auto line = [&font, &pos](const string &text, const Color &color)
	{
		font.Draw(text, pos, color);
		pos.Y() += LINE;
	};

	line(facility.TrueName() + (holding ? " x" + to_string(count) : ""), bright);
	if(holding)
		line("Status: " + StatusString(holding->status), medium);
	line("Cost: " + Format::CreditString(facility.Cost(), false) + (holding ? " to expand" : ""), medium);
	if(facility.Upkeep())
		line("Upkeep: " + Format::CreditString(facility.Upkeep() * count, false) + " per day", medium);
	if(!facility.Inputs().empty())
		line("Uses: " + AmountsString(facility.Inputs(), count) + " per day", medium);
	if(!facility.Outputs().empty())
		line("Makes: " + AmountsString(facility.Outputs(), count) + " per day", medium);

	WrappedText text(font);
	text.SetWrapWidth(width);
	if(holding)
	{
		vector<string> parts;
		for(const Facility::Amounts *amounts : {&facility.Inputs(), &facility.Outputs()})
			for(const auto &[commodity, amount] : *amounts)
				parts.push_back(commodity + " " + to_string(holding->Stock(commodity))
					+ "/" + to_string(holding->Capacity()));
		text.Wrap("Stock: " + Join(parts) + (holding->autoSell ? ". Output is auto-sold." : "."));
	}
	else
		text.Wrap(facility.Description());
	pos.Y() += 4.;
	text.Draw(pos, medium);
	pos.Y() += text.Height();

	// The latest report from this facility, if there is room for it above the buttons.
	if(holding && !holding->reports.empty())
	{
		const auto &[date, report] = holding->reports.back();
		text.Wrap(date + ": " + report);
		const double buttonsTop = box.Bottom() - LINE - BUTTON_SIZE.Y() - 8.;
		if(pos.Y() + text.Height() <= buttonsTop)
			text.Draw(pos, medium);
	}

	// Action buttons along the bottom of the details.
	Point buttonCorner(left, box.Bottom() - LINE - BUTTON_SIZE.Y() - 8.);
	auto button = [&](const string &label, bool enabled, function<void()> action)
	{
		const Rectangle area = Rectangle::FromCorner(buttonCorner, BUTTON_SIZE);
		FillShader::Fill(area, enabled ? dim : faint);
		font.Draw(label, area.Center() - .5 * Point(font.Width(label), font.Height()), enabled ? bright : dim);
		AddZone(area, action);
		buttonCorner.X() += BUTTON_SIZE.X() + BUTTON_GAP;
	};
	const CargoHold &cargo = player.Cargo();
	button(holding ? "Expand" : "Build", player.Accounts().Credits() >= facility.Cost(), [this]() { Build(); });
	if(holding)
	{
		bool canSupply = false;
		for(const auto &[commodity, amount] : facility.Inputs())
			canSupply |= (cargo.Get(commodity) > 0 && holding->Stock(commodity) < holding->Capacity());
		if(!facility.Inputs().empty())
			button("Supply", canSupply, [this]() { Supply(); });
		button("Collect", holding->OutputStock() > 0 && cargo.Free() > 0, [this]() { Collect(); });
		button(holding->autoSell ? "Sell: on" : "Sell: off", CanAutoSell(), [this]() { ToggleAutoSell(); });
	}
}



void IndustryPanel::DrawOverview()
{
	const Rectangle box = ContentBox();
	const Font &font = FontSet::Get(14);
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const vector<Industry::Holding> &holdings = player.GetIndustry().Holdings();
	int64_t upkeep = 0;
	for(const Industry::Holding &holding : holdings)
		upkeep += holding.type->Upkeep() * holding.count;

	font.Draw("All holdings", box.TopLeft() + Point(PAD, 0.), bright);
	const string total = "Upkeep: " + Format::CreditString(upkeep, false) + " per day";
	font.Draw(total, Point(box.Right() - PAD - font.Width(total), box.Top()), medium);

	if(holdings.empty())
	{
		font.Draw("You don't own any facilities yet.", box.TopLeft() + Point(PAD, LINE + 6.), medium);
		return;
	}

	const double columns[] = {box.Left() + PAD, box.Left() + 130., box.Left() + 300.};
	double y = box.Top() + LINE + 6.;
	font.Draw("Planet", Point(columns[0], y), dim);
	font.Draw("Facility", Point(columns[1], y), dim);
	font.Draw("Status", Point(columns[2], y), dim);
	const string goodsHeading = "Goods";
	font.Draw(goodsHeading, Point(box.Right() - PAD - font.Width(goodsHeading), y), dim);
	y += ROW;

	const int rows = VisibleRows(box, y);
	scroll = clamp(scroll, 0, max(0, static_cast<int>(holdings.size()) - rows));
	for(int i = scroll; i < static_cast<int>(holdings.size()) && i < scroll + rows; ++i)
	{
		const Industry::Holding &holding = holdings[i];
		const Planet *where = GameData::Planets().Find(holding.planet);
		const bool isHere = (where == &planet);
		const Color &color = isHere ? bright : medium;
		font.Draw(where ? where->DisplayName() : holding.planet, Point(columns[0], y), color);
		font.Draw(holding.type->TrueName() + " x" + to_string(holding.count), Point(columns[1], y), color);
		font.Draw(StatusString(holding.status), Point(columns[2], y), color);
		const string goods = holding.autoSell ? "auto-sold" : Format::MassString(holding.OutputStock());
		font.Draw(goods, Point(box.Right() - PAD - font.Width(goods), y), color);
		y += ROW;
	}
}



void IndustryPanel::Build()
{
	const Facility *facility = Selected();
	if(!facility)
		return;
	const int64_t missing = facility->Cost() - player.Accounts().Credits();
	if(missing > 0)
	{
		status = "You need " + Format::CreditString(missing, false) + " more.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	player.Accounts().AddCredits(-facility->Cost());
	Industry &industry = player.GetIndustry();
	industry.Build(*facility, planet.TrueName());
	const int count = industry.Find(*facility, planet.TrueName())->count;
	status = (count == 1 ? "Built a " + facility->TrueName() + " on " + planet.DisplayName()
		: "Expanded your " + facility->TrueName() + " to " + to_string(count) + " units") + ".";
	Messages::Add({status, GameData::MessageCategories().Get("normal")});
	UI::PlaySound(UI::UISound::NORMAL);
}



void IndustryPanel::Supply()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding)
		return;
	if(facility->Inputs().empty())
	{
		status = "This facility doesn't need any supplies.";
		return;
	}

	CargoHold &cargo = player.Cargo();
	vector<string> supplied;
	vector<string> missing;
	for(const auto &[commodity, amount] : facility->Inputs())
	{
		const int amountSupplied = Industry::Supply(*holding, commodity, cargo.Get(commodity));
		if(amountSupplied)
		{
			// Remove the cost basis of the goods along with the goods themselves.
			player.AdjustBasis(commodity, -player.GetBasis(commodity, amountSupplied));
			cargo.Remove(commodity, amountSupplied);
			supplied.push_back(Format::CargoString(amountSupplied, commodity));
		}
		else if(holding->Stock(commodity) < holding->Capacity())
			missing.push_back(commodity);
	}
	if(!supplied.empty())
	{
		status = "Supplied " + Join(supplied) + ".";
		UI::PlaySound(UI::UISound::NORMAL);
	}
	else
	{
		status = missing.empty() ? "Input storage is already full."
			: "Your cargo hold has no " + Join(missing) + ".";
		UI::PlaySound(UI::UISound::FAILURE);
	}
}



void IndustryPanel::Collect()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding)
		return;

	CargoHold &cargo = player.Cargo();
	if(!holding->OutputStock())
		status = "Nothing to collect yet. Check back in a few days.";
	else if(cargo.Free() <= 0)
		status = "Your fleet has no free cargo space.";
	else
	{
		vector<string> loaded;
		for(const auto &[commodity, amount] : facility->Outputs())
		{
			const int taken = Industry::Collect(*holding, commodity, cargo.Free());
			if(taken)
			{
				cargo.Add(commodity, taken);
				loaded.push_back(Format::CargoString(taken, commodity));
			}
		}
		status = "Loaded " + Join(loaded) + " into your cargo hold.";
		UI::PlaySound(UI::UISound::NORMAL);
		return;
	}
	UI::PlaySound(UI::UISound::FAILURE);
}



void IndustryPanel::ToggleAutoSell()
{
	const Facility *facility = Selected();
	Industry::Holding *holding = facility ? player.GetIndustry().Find(*facility, planet.TrueName()) : nullptr;
	if(!holding)
		return;
	if(!CanAutoSell())
	{
		status = "There is no market here to sell to.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	holding->autoSell = !holding->autoSell;
	status = holding->autoSell ? "Output will be sold to the local market every day."
		: "Output will be stored for you to collect.";
	UI::PlaySound(UI::UISound::NORMAL);
}



bool IndustryPanel::CanAutoSell() const
{
	const System *system = planet.GetSystem();
	return planet.IsInhabited() && planet.GetPort().HasService(Port::ServicesType::Trading)
		&& system && system->HasTrade();
}
