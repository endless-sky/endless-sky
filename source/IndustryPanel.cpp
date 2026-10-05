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
#include "Rectangle.h"
#include "Screen.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>

using namespace std;

namespace {
	const double ROW_HEIGHT = 46.;
	const double PAD = 10.;
	const Point BUTTON_SIZE(90., 24.);

	string OutputString(const Facility &facility)
	{
		return "Produces " + Format::CargoString(facility.OutputPerDay(), facility.Output()) + " per day.";
	}
}



IndustryPanel::IndustryPanel(PlayerInfo &player, const Planet &planet)
	: player(player), planet(planet)
{
	// Let the planet panel underneath keep handling its own buttons and keys.
	SetTrapAllEvents(false);
	SetInterruptible(false);
}



bool IndustryPanel::HasIndustry(const PlayerInfo &player, const Planet &planet)
{
	return !Facilities(player, planet).empty();
}



void IndustryPanel::Draw()
{
	ClearZones();

	const Interface *planetUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "planet (small screen)" : "planet");
	const Rectangle box = planetUi->GetBox("content");

	const Font &font = FontSet::Get(14);
	const Color &faint = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");

	const vector<const Facility *> facilities = Facilities(player, planet);
	if(facilities.empty())
		return;
	selectedRow = clamp(selectedRow, 0, static_cast<int>(facilities.size()) - 1);

	Point corner = box.TopLeft();
	font.Draw("Industry on " + planet.DisplayName(), corner + Point(PAD, 0.), bright);
	corner.Y() += 25.;

	const Industry &industry = player.GetIndustry();
	for(size_t i = 0; i < facilities.size(); ++i)
	{
		const Facility &facility = *facilities[i];
		const Industry::Holding *holding = industry.Find(facility, planet.TrueName());
		const bool isSelected = (static_cast<int>(i) == selectedRow);

		const Rectangle row = Rectangle::FromCorner(corner, Point(box.Width(), ROW_HEIGHT));
		if(isSelected)
			FillShader::Fill(row, faint);
		AddZone(row, [this, i]() { selectedRow = i; });

		// Name and status of the facility.
		string name = facility.TrueName() + (holding ? " (owned)" : "");
		string details = OutputString(facility) + " ";
		if(holding)
			details += "Stockpile: " + to_string(holding->stockpile) + " / "
				+ Format::MassString(facility.Storage()) + ".";
		else
			details += "Cost: " + Format::CreditString(facility.Cost(), false) + ".";
		font.Draw(name, corner + Point(PAD, 5.), isSelected ? bright : medium);
		font.Draw(details, corner + Point(PAD, 25.), medium);

		// Action button on the right side of the row.
		bool canAct = holding ? (holding->stockpile > 0 && player.Cargo().Free() > 0)
			: (player.Accounts().Credits() >= facility.Cost());
		const Point buttonCenter(box.Right() - PAD - .5 * BUTTON_SIZE.X(), corner.Y() + .5 * ROW_HEIGHT);
		const Rectangle button(buttonCenter, BUTTON_SIZE);
		FillShader::Fill(button, canAct ? dim : faint);
		const string label = holding ? "Collect" : "Build";
		font.Draw(label, buttonCenter - .5 * Point(font.Width(label), font.Height()), canAct ? bright : dim);
		AddZone(button, [this, i, &facility]() { selectedRow = i; DoAction(facility); });

		corner.Y() += ROW_HEIGHT;
	}

	// Description of the selected facility.
	WrappedText text(font);
	text.SetWrapWidth(box.Width() - 2. * PAD);
	text.Wrap(facilities[selectedRow]->Description());
	text.Draw(corner + Point(PAD, 10.), medium);

	if(!status.empty())
		font.Draw(status, Point(box.Left() + PAD, box.Bottom() - font.Height() - 5.), bright);
}



bool IndustryPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	const vector<const Facility *> facilities = Facilities(player, planet);
	if(facilities.empty())
		return false;

	if(key == SDLK_UP)
		selectedRow = max(0, selectedRow - 1);
	else if(key == SDLK_DOWN)
		selectedRow = min(static_cast<int>(facilities.size()) - 1, selectedRow + 1);
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER)
		DoAction(*facilities[clamp(selectedRow, 0, static_cast<int>(facilities.size()) - 1)]);
	else
		return false;

	return true;
}



vector<const Facility *> IndustryPanel::Facilities(const PlayerInfo &player, const Planet &planet)
{
	vector<const Facility *> result;
	for(const auto &it : GameData::Facilities())
	{
		const Facility &facility = it.second;
		if(facility.IsDefined() && (facility.CanBuildOn(planet.TrueName())
				|| player.GetIndustry().Find(facility, planet.TrueName())))
			result.push_back(&facility);
	}
	return result;
}



void IndustryPanel::DoAction(const Facility &facility)
{
	Industry &industry = player.GetIndustry();
	Industry::Holding *holding = industry.Find(facility, planet.TrueName());
	if(!holding)
	{
		int64_t missing = facility.Cost() - player.Accounts().Credits();
		if(missing > 0)
		{
			status = "You need " + Format::CreditString(missing, false) + " more to build this.";
			UI::PlaySound(UI::UISound::FAILURE);
			return;
		}
		player.Accounts().AddCredits(-facility.Cost());
		industry.Build(facility, planet.TrueName());
		status = "Built a " + facility.TrueName() + " on " + planet.DisplayName() + ".";
		Messages::Add({status, GameData::MessageCategories().Get("normal")});
		UI::PlaySound(UI::UISound::NORMAL);
		return;
	}

	if(!holding->stockpile)
	{
		status = "Nothing to collect yet. Check back in a few days.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	CargoHold &cargo = player.Cargo();
	int amount = Industry::Collect(*holding, cargo.Free());
	if(!amount)
	{
		status = "Your fleet has no free cargo space.";
		UI::PlaySound(UI::UISound::FAILURE);
		return;
	}
	cargo.Add(facility.Output(), amount);
	status = "Loaded " + Format::CargoString(amount, facility.Output()) + " into your cargo hold.";
	UI::PlaySound(UI::UISound::NORMAL);
}
