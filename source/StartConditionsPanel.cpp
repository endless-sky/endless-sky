/* StartConditionsPanel.cpp
Copyright (c) 2020-2021 by FranchuFranchu <fff999abc999@gmail.com>
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StartConditionsPanel.h"

#include "text/alignment.hpp"
#include "Command.h"
#include "ConversationPanel.h"
#include "text/DisplayText.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "MainPanel.h"
#include "PlayerInfo.h"
#include "Planet.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "ShipyardPanel.h"
#include "StarField.h"
#include "StartConditions.h"
#include "System.h"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

using namespace std;



StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels, const StartConditionsList &scenarios, const Panel *parent)
	: player(player), gamePanels(gamePanels), scenarios(scenarios), parent(parent),
	bright(*GameData::Colors().Get("bright")), medium(*GameData::Colors().Get("medium")),
	selectedBackground(*GameData::Colors().Get("faint")),
	description(FontSet::Get(14)),
	startIt(scenarios.end())
{
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
	
	const auto startCount = scenarios.size();
	if(startCount >= 1)
	{
		// Default the selection to the most recently loaded scenario.
		startIt = scenarios.end() - 1;
		ScrollToSelected();
	}
	
	const Rectangle firstRectangle = Rectangle::FromCorner(entriesContainer.TopLeft(), entryBox.Dimensions());
	startConditionsClickZones.reserve(startCount);
	for(size_t i = 0; i < startCount; ++i)
		startConditionsClickZones.emplace_back(firstRectangle + Point(0, i * entryBox.Height()), scenarios.begin() + i);
	
	description.SetAlignment(Alignment::LEFT);
	description.SetWrapWidth(descriptionBox.Width());
	description.Wrap(startIt != scenarios.end() ? startIt->GetDescription()
		: "No valid starting scenarios were defined!\n\n"
		"Make sure you installed Endless Sky (and any plugins) properly.");
}



void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	if(startIt != scenarios.end())
	{
		info.SetCondition("chosen start");
		if(startIt->GetThumbnail())
			info.SetSprite("thumbnail", startIt->GetThumbnail());
		info.SetString("name", startIt->GetDisplayName());
		info.SetString("description", startIt->GetDescription());
		info.SetString("planet", startIt->GetPlanet().Name());
		info.SetString("system", startIt->GetSystem().Name());
		info.SetString("date", startIt->GetDate().ToString());
		info.SetString("credits", Format::Credits(startIt->GetAccounts().Credits()));
		info.SetString("debt", Format::Credits(startIt->GetAccounts().TotalDebt()));
	}
	
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);
	
	// Start at the top left of the list and offset by the text margins and scroll.
	auto pos = entriesContainer.TopLeft() - Point(0., entriesScroll);
	
	const Font &font = FontSet::Get(14);
	for(auto it = scenarios.begin(); it != scenarios.end();
			++it, pos += Point(0., entryBox.Height()))
	{
		// TODO: replace skips with alpha fade-in/fade-out.
		if(!entriesContainer.Contains(pos) && !entriesContainer.Contains(pos + entryTextPadding))
			continue;
		
		const auto zone = Rectangle::FromCorner(pos, entryBox.Dimensions());
		bool isHighlighted = it == startIt || zone.Contains(hoverPoint);
		if(it == startIt)
			FillShader::Fill(zone.Center(), zone.Dimensions(), selectedBackground);
		
		const auto name = DisplayText(it->GetDisplayName(), Truncate::BACK);
		font.Draw(name, pos + entryTextPadding, isHighlighted ? bright : medium);
	}
	
	// TODO: Prevent lengthy descriptions from overflowing.
	description.Draw(descriptionBox.TopLeft(), bright);
}



bool StartConditionsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool /* isNewPress */)
{
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) 
	{
		ptrdiff_t offset = (key == SDLK_DOWN) - (key == SDLK_UP);
		offset += (entriesContainer.Height() / entryBox.Height()) * ((key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP));
		
		if(scenarios.begin() - startIt > offset)
			startIt = scenarios.begin();
		else if(scenarios.end() - startIt <= offset)
			startIt = scenarios.end() - 1;
		else
			startIt += offset;
		
		ScrollToSelected();
		description.Wrap(startIt->GetDescription());
	}
	else if(startIt != scenarios.end() && (key == 's' || key == 'n' || key == SDLK_KP_ENTER || key == SDLK_RETURN))
	{
		player.New(*startIt);
		
		ConversationPanel *panel = new ConversationPanel(
			player, startIt->GetConversation());
		GetUI()->Push(panel);
		panel->SetCallback(this, &StartConditionsPanel::OnConversationEnd);
	}
	else 
		return false;
	
	return true;
}



bool StartConditionsPanel::Click(int x, int y, int /* clicks */)
{
	// Only clicks within the list of scenarios should have an effect.
	if(!entriesContainer.Contains(Point(x, y)))
		return false;
	
	for(const auto &it : startConditionsClickZones)
		if(it.Contains(Point(x, y + entriesScroll)))
		{
			if(startIt != it.Value())
			{
				descriptionScroll = 0;
				startIt = it.Value();
				description.Wrap(startIt->GetDescription());
			}
			return true;
		}
	
	return false;
}



bool StartConditionsPanel::Hover(int x, int y)
{
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
		gamePanels.Push(new ShipyardPanel(player));
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
	
	const double maxScroll = startCount * entryHeight;
	const double pageScroll = maxDisplayedRows * entryHeight;
	const double desiredScroll = countBefore * 20.;
	const double bottomOfPage = entriesScroll + pageScroll;
	if(desiredScroll < entriesScroll)
	{
		// Scroll upwards.
		entriesScroll = desiredScroll;
	}
	else if(desiredScroll > bottomOfPage)
	{
		// Scroll downwards (but not so far that we overscroll).
		entriesScroll = min(maxScroll, entriesScroll + (desiredScroll - bottomOfPage));
	}
}
