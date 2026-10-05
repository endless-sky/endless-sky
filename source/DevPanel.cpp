/* DevPanel.cpp
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

#include "DevPanel.h"

#include "audio/Audio.h"
#include "Color.h"
#include "Command.h"
#include "DialogPanel.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Rectangle.h"
#include "UI.h"

using namespace std;

namespace {
	const double WIDTH = 420.;
	const double ROW_HEIGHT = 30.;
	const double PAD = 20.;
	// Space reserved above the rows for the title, and below them for the status line.
	const double HEADER = 50.;
	const double FOOTER = 60.;

	double Height(size_t rows)
	{
		return HEADER + rows * ROW_HEIGHT + FOOTER;
	}
}



DevPanel::DevPanel(PlayerInfo &player)
	: player(player)
{
	SetInterruptible(false);

	entries = {
		{"Advance 1 day", [this]() { AdvanceDays(1); }},
		{"Advance 7 days", [this]() { AdvanceDays(7); }},
		{"Advance 30 days", [this]() { AdvanceDays(30); }},
		{"Add 100,000 credits", [this]() { AddCredits(100000); }},
		{"Add 1,000,000 credits", [this]() { AddCredits(1000000); }},
		{"Add custom credits...", [this]() {
			GetUI().Push(DialogPanel::RequestInteger(this, &DevPanel::AddCustomCredits,
				"How many credits do you want to add? (Negative numbers remove credits.)"));
		}},
		{"Close", [this]() { GetUI().Pop(this); }},
	};
}



void DevPanel::Draw()
{
	DrawBackdrop();
	ClearZones();

	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);
	const Color &back = *GameData::Colors().Get("panel background");
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &highlight = *GameData::Colors().Get("faint");

	const double height = Height(entries.size());
	const Point topLeft(-.5 * WIDTH, -.5 * height);
	FillShader::Fill(Rectangle::FromCorner(topLeft, Point(WIDTH, height)), back);

	bigFont.Draw("Developer Menu", topLeft + Point(PAD, PAD), bright);

	Point rowCorner = topLeft + Point(0., HEADER);
	for(size_t i = 0; i < entries.size(); ++i)
	{
		const Rectangle row = Rectangle::FromCorner(rowCorner, Point(WIDTH, ROW_HEIGHT));
		if(static_cast<int>(i) == hoverIndex)
			FillShader::Fill(row, highlight);
		const string label = to_string(i + 1) + ". " + entries[i].label;
		font.Draw(label, rowCorner + Point(PAD, .5 * (ROW_HEIGHT - font.Height())),
			static_cast<int>(i) == hoverIndex ? bright : medium);
		AddZone(row, [this, i]() { Run(i); });
		rowCorner.Y() += ROW_HEIGHT;
	}

	// Status line: today's date, current credits, and what the last action did.
	Point status = rowCorner + Point(PAD, 10.);
	font.Draw(player.GetDate().ToString() + "   |   "
		+ Format::CreditString(player.Accounts().Credits(), false), status, bright);
	if(!lastResult.empty())
		font.Draw(lastResult, status + Point(0., 20.), medium);
}



bool DevPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_ESCAPE || key == '`' || key == SDLK_F12 || command.Has(Command::MENU))
		GetUI().Pop(this);
	else if(key >= '1' && key < static_cast<SDL_Keycode>('1' + entries.size()))
		Run(key - '1');
	else
		return false;

	return true;
}



bool DevPanel::Hover(int x, int y)
{
	const double height = Height(entries.size());
	const double top = -.5 * height + HEADER;
	hoverIndex = -1;
	if(x >= -.5 * WIDTH && x < .5 * WIDTH && y >= top)
	{
		const int row = static_cast<int>((y - top) / ROW_HEIGHT);
		if(row < static_cast<int>(entries.size()))
			hoverIndex = row;
	}
	return true;
}



void DevPanel::AdvanceDays(int days)
{
	// AdvanceDate runs the normal daily logic (events, mission deadlines,
	// salaries and debt payments), exactly as if the player had jumped.
	player.AdvanceDate(days);
	lastResult = "Advanced " + Format::SimplePluralization(days, "day") + ".";
	Messages::Add({"[Dev] " + lastResult + " Today is " + player.GetDate().ToString() + ".",
		GameData::MessageCategories().Get("normal")});
}



void DevPanel::AddCredits(int64_t amount)
{
	player.Accounts().AddCredits(amount);
	lastResult = (amount >= 0 ? "Added " : "Removed ") + Format::CreditString(amount >= 0 ? amount : -amount, false) + ".";
}



void DevPanel::AddCustomCredits(int amount)
{
	AddCredits(amount);
}



void DevPanel::Run(size_t index)
{
	if(index >= entries.size())
		return;
	UI::PlaySound(UI::UISound::NORMAL);
	// Copy the action, since "Close" pops this panel while it runs.
	auto action = entries[index].action;
	action();
}
