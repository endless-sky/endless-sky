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

namespace {
	const int PAGE_INTERVAL = 10;
}

StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels, const Panel *parent)
	: player(player), gamePanels(gamePanels), descriptionWrappedText(FontSet::Get(14)), parent(parent)
{
	const Interface *startConditionsMenu = GameData::Interfaces().Find("start conditions menu");
	
	if(startConditionsMenu)	
	{
		// Ideally, we want the content of the boxes to be drawn in Interface.cpp.
		// However, we'd need a way to specify arbitrarily extensible lists there.
		// We also would need to a way to specify truncation for such a list.
		// Such a list would also have to be scrollable.
		descriptionBox = startConditionsMenu->GetBox("start description");
		entryBox = startConditionsMenu->GetBox("start entry");
		entryListBox = startConditionsMenu->GetBox("start entry list");
		entryInternalBox = startConditionsMenu->GetBox("start entry internal");	
	}
	
	if(!GameData::StartOptions().empty())
	{
		// Default the selection to the most recently loaded scenario.
		chosenStart = &GameData::StartOptions().back();
		chosenStartIterator = GameData::StartOptions().end() - 1;
		listScroll = (chosenStartIterator - GameData::StartOptions().begin()) * entryBox.Height();
	}
	
	const Rectangle firstRectangle = Rectangle::FromCorner(entryListBox.TopLeft(), entryBox.Dimensions());
	startConditionsClickZones.reserve(GameData::StartOptions().size());
	for(size_t i = 0; i < GameData::StartOptions().size(); ++i)
		startConditionsClickZones.emplace_back(firstRectangle + Point(0, i * entryBox.Height()), GameData::StartOptions().begin() + i);
	
	descriptionWrappedText.SetAlignment(Alignment::LEFT);
	descriptionWrappedText.SetWrapWidth(descriptionBox.Width());
	
	if(chosenStart)
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
	else 
		descriptionWrappedText.Wrap("No valid start scenarios were defined!\n\nMake sure that you installed Endless Sky and all of your plugins properly");
	
	bright = *GameData::Colors().Get("bright");
	medium = *GameData::Colors().Get("medium");
	selectedBackground = *GameData::Colors().Get("selected start conditions background");
}



// Called when the conversation ends.
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
	// It's possible that the player got to this menu directly from the main menu, so we need to check for that.
	if(parent)
		GetUI()->Pop(parent);
	
	GetUI()->Pop(GetUI()->Root().get());
	GetUI()->Pop(this);
}



void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	Information info;
	
	// String that will be shown in the description panel.
	string descriptionText; 
	
	if(chosenStart) 
	{
		info.SetCondition("chosen start");
		if(chosenStart->GetThumbnail())
			info.SetSprite("thumbnail", chosenStart->GetThumbnail());
		info.SetString("name", chosenStart->GetName());
		info.SetString("description", chosenStart->GetDescription());
		info.SetString("planet", chosenStart->GetPlanet().Name());
		info.SetString("system", chosenStart->GetSystem().Name());
		info.SetString("date", chosenStart->GetDate().ToString());
		info.SetString("credits", Format::Credits(chosenStart->GetAccounts().Credits()));
		info.SetString("debt", Format::Credits(chosenStart->GetAccounts().TotalDebt()));

		descriptionText = chosenStart->GetDescription();
	}
	
	GameData::Background().Draw(Point(), Point());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);
	
	// TODO: Prevent text from overflowing.
	descriptionWrappedText.Draw(
		Point(descriptionBox.Left(), descriptionBox.Top() + descriptionScroll),
		bright
	);
	
	Point point(entryListBox.Left(), entryListBox.Top() - listScroll);
	const Point offset = entryInternalBox.Dimensions() * .5;
	
	const Font &font = FontSet::Get(14);
	for(const auto &it : GameData::StartOptions())
	{
		Rectangle zone(
			point + offset, 
			entryBox.Dimensions()
		);
		if(point.Y() > entryListBox.Bottom() || point.Y() < entryListBox.Top())
		{
			// Don't bother drawing if the item is above or under the list.
			point += Point(0., entryBox.Height());
			continue;
		}
		
		bool isHighlighted = (&it == chosenStart);
		
		if(isHighlighted)
			FillShader::Fill(zone.Center(), zone.Dimensions(), selectedBackground);
		
		DisplayText name = DisplayText(it.GetName(), Truncate::BACK);
		font.Draw(name, point, isHighlighted ? bright : medium);
		point += Point(0., entryBox.Height());
	}
}



bool StartConditionsPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	
	if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		GetUI()->Pop(this);
	else if(key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) 
	{
		ptrdiff_t offset = 0;
		offset += (key == SDLK_DOWN) - (key == SDLK_UP);
		offset += PAGE_INTERVAL * ((key == SDLK_PAGEDOWN) - (key == SDLK_PAGEUP));
		
		DoScrolling(offset * entryBox.Height());
		
		if(GameData::StartOptions().begin() - chosenStartIterator > offset)
			chosenStartIterator = GameData::StartOptions().begin();
		else if(GameData::StartOptions().end() - chosenStartIterator <= offset)
			chosenStartIterator = GameData::StartOptions().end() - 1;
		else
			chosenStartIterator += offset;
		
		chosenStart = &*chosenStartIterator;
		descriptionWrappedText.Wrap(chosenStart->GetDescription());
		
		return true;
	}
	else if(key == 's' || key == 'n' || key == '\n')
	{
		if(!chosenStart)
			return false;
		
		player.New(*chosenStart);
	
		ConversationPanel *panel = new ConversationPanel(
			player, chosenStart->GetConversation());
		GetUI()->Push(panel);
		panel->SetCallback(this, &StartConditionsPanel::OnCallback);
		
		return true;
	}
	else 
		return true;
	return false;
}



bool StartConditionsPanel::Click(int x, int y, int clicks)
{
	// Check it's inside of the entry list box.
	if(!entryListBox.Contains(Point(x, y)))
		return true;
	
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
	
	return false;
}



bool StartConditionsPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	return false;
}



bool StartConditionsPanel::Drag(double dx, double dy)
{
	if(entryListBox.Contains(hoverPoint))
	{
		DoScrolling(-dy);
	}
	// TODO: When #4123 gets merged, re-add scrolling support to the description.
	// Right now it's pointless because it would make the text overflow.
	
	return true;
}



bool StartConditionsPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



double StartConditionsPanel::DoScrolling(int scrollAmount)
{
	int originalScrolling = listScroll;
	listScroll += scrollAmount;
	listScroll = min(max(listScroll, 0.), (GameData::StartOptions().size() - 1) * entryBox.Height());
	
	return (listScroll - originalScrolling) / entryBox.Height();
}
