/* StartConditionsPanel.cpp
Copyright (c) 2020 by Michael Zahniser

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
#include "text/WrappedText.h"
#include "UI.h"

#include <SDL2/SDL.h>

#include "gl_header.h"

using namespace std;

StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels, LoadPanel *loadPanel)
	: player(player), gamePanels(gamePanels), loadPanel(loadPanel)
{
	if(!GameData::Start().empty())
	{
		chosenStart = GameData::Start().back();
		hasChosenStart = true;
	}
	const Interface *startConditionsMenu = GameData::Interfaces().Find("start conditions menu");
	
	if (startConditionsMenu)	
	{
		descriptionBox =   startConditionsMenu->GetBox("start description");
		entryBox =         startConditionsMenu->GetBox("start entry");
		entryListBox =     startConditionsMenu->GetBox("start entry list");
		entryInternalBox = startConditionsMenu->GetBox("start entry internal");	
	}
	
	size_t i = 0;
	// Fill up the startConditionsClickZones vector
	for(const StartConditions &it : GameData::Start())
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
	
	bright = *GameData::Colors().Get("bright");
}


void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	
	Information info;
	
	// String that will be shown in the description panel
	string descriptionText; 
	
	if(hasChosenStart){
		info.SetCondition("chosen start");
		if(chosenStart.GetSprite())
			info.SetSprite("start sprite", chosenStart.GetSprite());
		info.SetString("name", chosenStart.GetName());
		info.SetString("description", chosenStart.GetDescription());
		info.SetString("planet", chosenStart.GetPlanet()->Name());
		info.SetString("system", chosenStart.GetSystem()->Name());
		info.SetString("date", chosenStart.GetDate().ToString());
		info.SetString("credits", Format::Credits(chosenStart.GetAccounts().Credits()));

		descriptionText = chosenStart.GetDescription();
	}
	else if (!GameData::Start().size())
	{
		descriptionText = "No start scenarios were defined!\n\nMake sure that you installed Endless Sky and all of your plugins properly";
	}
	
	GameData::Background().Draw(Point(), Point());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);
	
	const Font &font = FontSet::Get(14);
	
	WrappedText text = WrappedText(font);
	
	text.SetAlignment(Alignment::LEFT);
	text.SetWrapWidth(210);
	text.Wrap(descriptionText);
	
	text.Draw(
		Point(descriptionBox.Left(), descriptionBox.Top() + descriptionScroll),
		descriptionBox,
		bright
	);
	
	Point point(
		entryListBox.Left(),
		entryListBox.Top() - listScroll 
	);
	
	for(const auto &it : GameData::Start())
	{
		Rectangle zone(
			point  + Point(
				(entryInternalBox.Width()) / 2, 
				(entryInternalBox.Height()) / 2), 
			entryBox.Dimensions()
		);
		if (point.Y() > entryListBox.Bottom() || point.Y() < entryListBox.Top())
		{
			// Don't bother drawing if the item is above or under the list
			point += Point(0., entryBox.Height());
			continue;
		}
		
		
		bool isHighlighted = (it == chosenStart);
		
		// double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
		double alpha = 1;
		
		if(it == chosenStart)
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
	int x = hoverPoint.X();
	if(x >= entryListBox.Left() && x < entryListBox.Right())
	{
		// Scroll the list
		// This looks inefficient but it probably gets optimized by the compiler
		listScroll -= dy;
		listScroll = min(entryBox.Height() * (GameData::Start().size()-1), listScroll); // Avoid people going too low
		listScroll = max(0.,listScroll); // Snap the list to avoid people scrolling too far up
	}
	else
	{
		// Scroll the description
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
	hoverPoint = Point(x,y);
	return true;
}



bool StartConditionsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == 's' || key == 'n' || key == '\n')
	{
		if(!hasChosenStart)
			return true;
		
		player.New(chosenStart);
		
		
		ConversationPanel *panel = new ConversationPanel(
			player, *GameData::Conversations().Get("intro"));
		GetUI()->Push(panel);
		panel->SetCallback(this, &StartConditionsPanel::OnCallback);
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
		if (!it.Contains(Point(x, y + listScroll)))
			continue;
		
		// We found the element we clicked on
		
		if(!(chosenStart == it.Value()))
			descriptionScroll = 0; // Reset scrolling
		chosenStart = it.Value();
		hasChosenStart = true;
	}
	
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
	if(loadPanel)
		GetUI()->Pop(loadPanel);
	
	GetUI()->Pop(GetUI()->Root().get());
	GetUI()->Pop(this);
}
