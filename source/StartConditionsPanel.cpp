/* StartConditionsPanel.cpp
Copyright (c) 2020 by FranchuFranchu <fff999abc999@gmail.com>

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StartConditionsPanel.h"

#include "Color.h"
#include "Command.h"
#include "ConversationPanel.h"
#include "DataFile.h"
#include "Dialog.h"
#include "Files.h"
#include "FillShader.h"
#include "GameData.h"
#include "GameWindow.h"
#include "Information.h"
#include "Interface.h"
#include "LoadPanel.h"
#include "MainPanel.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Planet.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "ShipyardPanel.h"
#include "StarField.h"
#include "System.h"
#include "text/alignment.hpp"
#include "text/DisplayText.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "text/layout.hpp"
#include "text/truncate.hpp"
#include "UI.h"

#include <algorithm>

using namespace std;

namespace {
	const int PAGE_INTERVAL = 10;
}

StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels, Panel *parent)
	: player(player), gamePanels(gamePanels), parent(parent)
{
	if(!GameData::StartOptions().empty())
	{
		// By default, select the last defined start conditions
		// (which are usually the ones defined by a plugin)
		chosenStart = &GameData::StartOptions().back();
		hasChosenStart = true;
	}
	const Interface *startConditionsMenu = GameData::Interfaces().Find("start conditions menu");
	
	if(startConditionsMenu)	
	{
		descriptionBox =   startConditionsMenu->GetBox("start description");
		entryBox =         startConditionsMenu->GetBox("start entry");
		entryListBox =     startConditionsMenu->GetBox("start entry list");
		entryInternalBox = startConditionsMenu->GetBox("start entry internal");	
	}
	
	size_t i = 0;
	// Fill up the startConditionsClickZones vector
	for(const StartConditions &it : GameData::StartOptions())
	{
		// emplace_back will implicitly instantiate a ClickZone
		startConditionsClickZones.emplace_back(
			Rectangle::FromCorner(
				Point(entryListBox.Left(), entryListBox.Top() + i * entryBox.Height()),
				entryBox.Dimensions()
			),
			it);
		i++;
	}
	const Font &font = FontSet::Get(14);
	
	descriptionWrappedText = WrappedText(font);
	
	descriptionWrappedText.SetAlignment(Alignment::LEFT);
	descriptionWrappedText.SetWrapWidth(descriptionBox.Width());
	
	if (hasChosenStart)
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
	
	bright = *GameData::Colors().Get("bright");
}


void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Font &font = FontSet::Get(14);
	
	Information info;
	
	// String that will be shown in the description panel
	string descriptionText; 
	
	if(hasChosenStart) 
	{
		info.SetCondition("chosen start");
		if(chosenStart->GetSprite())
			info.SetSprite("start sprite", chosenStart->GetSprite());
		info.SetString("name", chosenStart->GetName());
		info.SetString("description", chosenStart->GetDescription());
		info.SetString("planet", chosenStart->GetPlanet()->Name());
		info.SetString("system", chosenStart->GetSystem()->Name());
		info.SetString("date", chosenStart->GetDate().ToString());
		info.SetString("credits", Format::Credits(chosenStart->GetAccounts().Credits()));

		descriptionText = chosenStart->GetDescription();
	}
	else if(!GameData::StartOptions().size())
	{
		descriptionText = "No start scenarios were defined!\n\nMake sure that you installed Endless Sky and all of your plugins properly";
	}
	
	GameData::Background().Draw(Point(), Point());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);
	
	
	
	// TODO: Prevent text from overflowing	
	descriptionWrappedText.Draw(
		Point(descriptionBox.Left(), descriptionBox.Top() + descriptionScroll),
		bright
	);
	
	Point point(entryListBox.Left(), entryListBox.Top() - listScroll);
	
	for(const auto &it : GameData::StartOptions())
	{
		Rectangle zone(
			point  + Point(
				(entryInternalBox.Width()) / 2, 
				(entryInternalBox.Height()) / 2), 
			entryBox.Dimensions()
		);
		if(point.Y() > entryListBox.Bottom() || point.Y() < entryListBox.Top())
		{
			// Don't bother drawing if the item is above or under the list
			point += Point(0., entryBox.Height());
			continue;
		}
		
		
		bool isHighlighted = (&it == chosenStart);
		
		// double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
		double alpha = 1;
		
		if(&it == chosenStart)
		{
			FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
		}
		
		DisplayText name = DisplayText(it.GetName(), Truncate::BACK);
		font.Draw(name, point, Color((isHighlighted ? .7 : .5) * alpha, 0.));
		point += Point(0., entryBox.Height());
	}
}



bool StartConditionsPanel::Drag(double dx, double dy)
{
	if(entryListBox.Contains(hoverPoint))
	{
		listScroll -= dy;
		listScroll = min(entryBox.Height() * (GameData::StartOptions().size()-1), listScroll); 
		listScroll = max(0., listScroll);
	}
	else if(descriptionBox.Contains(hoverPoint))
	{		
		descriptionScroll += dy;
		descriptionScroll = min(0., descriptionScroll); // Snap the list to avoid people scrolling too far up
	}
	
	return true;
}



bool StartConditionsPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



bool StartConditionsPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	return true;
}



bool StartConditionsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	
	// To select the previous start conditions, we'll search for the selected start
	// conditions, and move the iterator backwards when we find them
	
	// Similar actions can be performed to select the next one
	
	auto chosenStartPosition = find(GameData::StartOptions().begin(), GameData::StartOptions().end(), *chosenStart);
	
	
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_UP) 
	{
		if (chosenStart == &*GameData::StartOptions().begin())
			return false;
		
		chosenStart = &*chosenStartPosition-1;
	}
	else if (key == SDLK_DOWN)
	{
		if (chosenStart == &*GameData::StartOptions().end() - 1)
			return false;
		
		chosenStart = &*chosenStartPosition+1;	
	}
	// When using the pageup and pagedown keys, it's slighly more complex
	// We'll select the StartConditions 10 (PAGE_INTERVAL) positions before or after the selected one
	// We have to make sure that the iterator we move doesn't go off bounds
	
	// In the future, we should consider dynamically calculating PAGE_INTERVAL based on the dimensions of the entryListBox,
	// but it's unlikely that the player will install enough plugins with start conditions at once 
	// so that such a feature were required
	else if(key == SDLK_PAGEUP) 
	{
		if ((chosenStartPosition - GameData::StartOptions().begin()) < PAGE_INTERVAL)
		{
			chosenStart = &GameData::StartOptions().front();
			return false;
		}
		
		chosenStart = &*chosenStartPosition-PAGE_INTERVAL;
	}
	else if(key == SDLK_PAGEDOWN) 
	{
		if ((GameData::StartOptions().end() - 1 - chosenStartPosition) < PAGE_INTERVAL)
		{
			chosenStart = &GameData::StartOptions().back();
			return false;
		}
		
		chosenStart = &*chosenStartPosition+PAGE_INTERVAL;
	}
	else if(key == 's' || key == 'n' || key == '\n')
	{
		if(!hasChosenStart)
			return true;
		
		player.New(*chosenStart);
		
		if(chosenStart->GetConversation().IsEmpty())
		{
			// If no conversation was defined, then skip the conversation panel
			OnCallback(0);
		}
		else
		{
			ConversationPanel *panel = new ConversationPanel(
				player, chosenStart->GetConversation());
			GetUI()->Push(panel);
			panel->SetCallback(this, &StartConditionsPanel::OnCallback);
		}
	}
	else 
		return true;
	return false;
}



bool StartConditionsPanel::Click(int x, int y, int clicks)
{
	// Check it's inside of the entry list box
	if(!entryListBox.Contains(Point(x, y)))
		return false;
	
	for(const auto &it : startConditionsClickZones)
	{
		if(it.Contains(Point(x, y + listScroll)))
		{
			if(chosenStart != &it.Value())
				descriptionScroll = 0;
			chosenStart = &it.Value();
			hasChosenStart = true;
			break;
		}
	}
	
	if (hasChosenStart)
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
	
	return true;
}



// Called when the conversation ends
void StartConditionsPanel::OnCallback(int)
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
	// It's possible that the player got to this menu directly from the main menu, so we need to check for that
	if(parent)
		GetUI()->Pop(parent);
	
	GetUI()->Pop(GetUI()->Root().get());
	GetUI()->Pop(this);
}
