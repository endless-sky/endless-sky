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
	: player(player), gamePanels(gamePanels), descriptionWrappedText(WrappedText(FontSet::Get(14))), parent(parent)
{
	if(!GameData::StartOptions().empty())
	{
		// By default, select the last defined start conditions
		// (which are usually the ones defined by a plugin)
		chosenStart = &GameData::StartOptions().back();
	}
	const Interface *startConditionsMenu = GameData::Interfaces().Find("start conditions menu");
	
	if(startConditionsMenu)	
	{
		// Ideally, we want the content of the boxes to be drawn in Interface.cpp
		// However, we'd need a way to specify arbitrarily extensible lists there
		// We also would need to a way to specify truncation for such a list
		// Such a list would also have to be scrollable
		descriptionBox =   startConditionsMenu->GetBox("start description");
		entryBox =         startConditionsMenu->GetBox("start entry");
		entryListBox =     startConditionsMenu->GetBox("start entry list");
		entryInternalBox = startConditionsMenu->GetBox("start entry internal");	
	}
	
	const Rectangle &firstRectangle = Rectangle::FromCorner(entryListBox.TopLeft(), entryBox.Dimensions());
	startConditionsClickZones.reserve(GameData::StartOptions().size());
	for(size_t i = 0; i < GameData::StartOptions().size(); ++i)
	{
		startConditionsClickZones.emplace_back(firstRectangle + Point(0, i * entryBox.Height()), GameData::StartOptions().begin() + i);
	}
	
	descriptionWrappedText.SetAlignment(Alignment::LEFT);
	descriptionWrappedText.SetWrapWidth(descriptionBox.Width());
	
	if(chosenStart)
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
	else 
	{
		descriptionWrappedText.Wrap("No valid start scenarios were defined!\n\nMake sure that you installed Endless Sky and all of your plugins properly");
	}
	bright = *GameData::Colors().Get("bright");
}


void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	const Font &font = FontSet::Get(14);
	
	Information info;
	
	// String that will be shown in the description panel
	string descriptionText; 
	
	if(chosenStart) 
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
		info.SetString("debt", Format::Credits(chosenStart->GetAccounts().TotalDebt()));

		descriptionText = chosenStart->GetDescription();
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
	// TODO: When #4123 gets merged, re-add scrolling support
	// Right now it's pointless because it would make the text overflow
	
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
	
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) 
	{
		int offset = 0;
		offset += (key == SDLK_DOWN) - (key == SDLK_UP);
		offset += PAGE_INTERVAL * ((key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP));
		
		listScroll += offset * entryBox.Height();
		listScroll = min(max(listScroll, 0.), (GameData::StartOptions().size() - 1) * entryBox.Height());
		
		chosenStartIterator += offset;
		
		if(chosenStartIterator < GameData::StartOptions().begin())
		{
			chosenStartIterator = GameData::StartOptions().begin();
		}
		
		if(chosenStartIterator >= GameData::StartOptions().end())
		{
			chosenStartIterator = GameData::StartOptions().end() - 1;
		}
		
		
		chosenStart = &*chosenStartIterator;
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
		
		return false;
	}
	else if(key == 's' || key == 'n' || key == '\n')
	{
		if(!chosenStart)
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
			chosenStartIterator = it.Value();
			
			if(chosenStart != &*chosenStartIterator)
				descriptionScroll = 0;
			chosenStart = &*chosenStartIterator;
			descriptionWrappedText.Wrap(chosenStart->GetDescription());
			break;
		}
	}
	
	if(chosenStart)
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
