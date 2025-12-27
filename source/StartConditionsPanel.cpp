/* StartConditionsPanel.cpp
Copyright (c) 2020-2021 by FranchuFranchu <fff999abc999@gmail.com>
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "StartConditionsPanel.h"

#include "Command.h"
#include "ConversationPanel.h"
#include "text/DisplayText.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "MainPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "Ship.h"
#include "ShipyardPanel.h"
#include "Shop.h"
#include "shader/StarField.h"
#include "StartConditions.h"
#include "System.h"
#include "text/Truncate.h"
#include "UI.h"

#include <algorithm>

using namespace std;



StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels,
	const StartConditionsList &allScenarios, const Panel *parent)
	: player(player), gamePanels(gamePanels), parent(parent),
	bright(*GameData::Colors().Get("bright")), medium(*GameData::Colors().Get("medium")),
	selectedBackground(*GameData::Colors().Get("faint")),
	description(FontSet::Get(14))
{
	// Extract from all start scenarios those that are visible to the player.
	for(const auto &scenario : allScenarios)
		if(scenario.Visible())
		{
			scenarios.emplace_back(scenario);
			scenarios.back().SetState();
		}

	startIt = scenarios.begin();

	const Interface *startConditionsMenu = GameData::Interfaces().Find("start conditions menu");
	if(startConditionsMenu)
	{
		// Ideally, we want the content of the boxes to be drawn in Interface.cpp.
		// However, we'd need a way to specify arbitrarily extensible lists there.
		// We also would need to a way to specify truncation for such a list.
		// Such a list would also have to be scrollable.
		descriptionBox = startConditionsMenu->GetBox("start description");
		entriesContainer = startConditionsMenu->GetBox("start entry list");
		entryBox = startConditionsMenu->GetBox("start entry item bounds");
		entryTextPadding = startConditionsMenu->GetBox("start entry text padding").Dimensions();
	}

	const Rectangle firstRectangle = Rectangle::FromCorner(entriesContainer.TopLeft(), entryBox.Dimensions());
	const auto startCount = scenarios.size();
	startConditionsClickZones.reserve(startCount);
	for(size_t i = 0; i < startCount; ++i)
		startConditionsClickZones.emplace_back(firstRectangle + Point(0, i * entryBox.Height()), scenarios.begin() + i);

	description.SetWrapWidth(descriptionBox.Width());

	Select(startIt);
}



void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);

	// Rather than blink list items in & out of existence, fade them in/out over half the entry height.
	const double fadeDistance = .5 * entryBox.Height();
	const double fadeInY = entriesContainer.Top() - fadeDistance + entryTextPadding.Y();
	const double fadeOutY = fadeInY + entriesContainer.Height();

	// Start at the top left of the list and offset by the text margins and scroll.
	auto pos = entriesContainer.TopLeft() - Point(0., entriesScroll);

	const Font &font = FontSet::Get(14);
	for(auto it = scenarios.begin(); it != scenarios.end();
			++it, pos += Point(0., entryBox.Height()))
	{
		// Any scenario wholly outside the bounds can be skipped.
		const auto zone = Rectangle::FromCorner(pos, entryBox.Dimensions());
		if(!(entriesContainer.Contains(zone.TopLeft()) || entriesContainer.Contains(zone.BottomRight())))
			continue;

		// Partially visible entries should fade in or out.
		double opacity = entriesContainer.Contains(zone) ? 1.
			: min(1., max(0., min(pos.Y() - fadeInY, fadeOutY - pos.Y()) / fadeDistance));

		bool isHighlighted = it == startIt || (hasHover && zone.Contains(hoverPoint));
		if(it == startIt)
			FillShader::Fill(zone, selectedBackground.Additive(opacity));

		const auto name = DisplayText(it->GetDisplayName(), Truncate::BACK);
		font.Draw(name, pos + entryTextPadding, (isHighlighted ? bright : medium).Transparent(opacity));
	}

	// TODO: Prevent lengthy descriptions from overflowing.
	description.Draw(descriptionBox.TopLeft(), bright);
}



bool StartConditionsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(!scenarios.empty() && (key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN))
	{
		// Move up / down an entry, or a page. If at the bottom / top, wrap around.
		const ptrdiff_t magnitude = (key == SDLK_UP || key == SDLK_DOWN) ? 1
				: entriesContainer.Height() / entryBox.Height() - 1;
		if(key == SDLK_UP || key == SDLK_PAGEUP)
		{
			if(startIt == scenarios.begin())
				startIt = scenarios.end() - 1;
			else
				startIt -= min(magnitude, distance(scenarios.begin(), startIt));
		}
		else
		{
			if(startIt + 1 == scenarios.end())
				startIt = scenarios.begin();
			else
				startIt += min(magnitude, distance(startIt, scenarios.end() - 1));
		}

		Select(startIt);
	}
	else if(startIt != scenarios.end() && (key == 's' || key == 'n' || key == SDLK_KP_ENTER || key == SDLK_RETURN)
		&& info.HasCondition("unlocked start"))
	{
		player.New(*startIt);

		ConversationPanel *panel = new ConversationPanel(
			player, startIt->GetConversation());
		GetUI()->Push(panel);
		panel->SetCallback(this, &StartConditionsPanel::OnConversationEnd);
		return true;
	}
	else
		return false;

	UI::PlaySound(UI::UISound::NORMAL);
	return true;
}



bool StartConditionsPanel::Click(int x, int y, MouseButton button, int /* clicks */)
{
	// When the user clicks, clear the hovered state.
	hasHover = false;

	if(button != MouseButton::LEFT)
		return false;

	// Only clicks within the list of scenarios should have an effect.
	if(!entriesContainer.Contains(Point(x, y)))
		return false;

	for(const auto &it : startConditionsClickZones)
		if(it.Contains(Point(x, y + entriesScroll)))
		{
			if(startIt != it.Value())
				Select(it.Value());
			UI::PlaySound(UI::UISound::NORMAL);
			return true;
		}

	return false;
}



bool StartConditionsPanel::Hover(int x, int y)
{
	hasHover = true;
	hoverPoint = Point(x, y);
	return true;
}



bool StartConditionsPanel::Drag(double /* dx */, double dy)
{
	if(entriesContainer.Contains(hoverPoint))
	{
		entriesScroll = max(0., min(entriesScroll - dy,
			scenarios.size() * entryBox.Height() - entriesContainer.Height()));
	}
	else if(descriptionBox.Contains(hoverPoint))
	{
		descriptionScroll = 0.;
		// TODO: When #4123 gets merged, re-add scrolling support to the description.
		// Right now it's pointless because it would make the text overflow.
	}
	else
		return false;

	return true;
}



bool StartConditionsPanel::Scroll(double /* dx */, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



// Transition from the completed "new pilot" conversation into the actual game.
void StartConditionsPanel::OnConversationEnd(int)
{
	gamePanels.Reset();
	gamePanels.CanSave(true);
	gamePanels.Push(new MainPanel(player));
	// Tell the main panel to re-draw itself (and pop up the planet panel).
	gamePanels.StepAll();
	// If the starting conditions don't specify any ships, let the player buy one.
	if(player.Ships().empty())
	{
		Sale<Ship> shipyardStock;
		for(const Shop<Ship> *shop : player.GetPlanet()->Shipyards())
			shipyardStock.Add(shop->Stock());
		gamePanels.Push(new ShipyardPanel(player, shipyardStock));
		gamePanels.StepAll();
	}
	if(parent)
		GetUI()->Pop(parent);

	GetUI()->Pop(GetUI()->Root().get());
	GetUI()->Pop(this);
}



// Scroll the selected starting condition into view, if necessary.
void StartConditionsPanel::ScrollToSelected()
{
	// If there are fewer starts than there are displayable starts, never scroll.
	const double entryHeight = entryBox.Height();
	const int maxDisplayedRows = entriesContainer.Height() / entryHeight;
	const auto startCount = scenarios.size();
	if(static_cast<int>(startCount) < maxDisplayedRows)
	{
		entriesScroll = 0.;
		return;
	}
	// Otherwise, scroll the minimum of the desired amount and the amount that
	// brings the scrolled-to edge within view.
	const auto countBefore = static_cast<size_t>(distance(scenarios.begin(), startIt));

	const double maxScroll = (startCount - maxDisplayedRows) * entryHeight;
	const double pageScroll = maxDisplayedRows * entryHeight;
	const double desiredScroll = countBefore * entryHeight;
	const double bottomOfPage = entriesScroll + pageScroll;
	if(desiredScroll < entriesScroll)
	{
		// Scroll upwards.
		entriesScroll = desiredScroll;
	}
	else if(desiredScroll + entryHeight > bottomOfPage)
	{
		// Scroll downwards (but not so far that we overscroll).
		entriesScroll = min(maxScroll, entriesScroll + entryHeight + desiredScroll - bottomOfPage);
	}
}



// Update the UI to reflect the given starting scenario.
void StartConditionsPanel::Select(StartConditionsList::iterator it)
{
	// Clear the displayed information.
	info = Information();

	startIt = it;
	if(startIt == scenarios.end())
	{
		// The only time we should be here is if there are no scenarios at all.
		description.Wrap("No valid starting scenarios were defined!\n\n"
			"Make sure you installed Endless Sky (and any plugins) properly.");
		return;
	}


	// Update the information summary.
	info.SetCondition("chosen start");
	if(startIt->IsUnlocked())
		info.SetCondition("unlocked start");
	if(startIt->GetThumbnail())
		info.SetSprite("thumbnail", startIt->GetThumbnail());
	info.SetString("name", startIt->GetDisplayName());
	info.SetString("description", startIt->GetDescription());
	info.SetString("planet", startIt->GetPlanetName());
	info.SetString("system", startIt->GetSystemName());
	info.SetString("date", startIt->GetDateString());
	info.SetString("credits", startIt->GetCredits());
	info.SetString("debt", startIt->GetDebt());


	// Update the displayed description text.
	descriptionScroll = 0;
	description.Wrap(startIt->GetDescription());

	// Scroll the selected scenario into view.
	ScrollToSelected();
}
