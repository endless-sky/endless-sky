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
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
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
#include "Ship.h"
#include "ShipyardPanel.h"
#include "StarField.h"
#include "System.h"
#include "UI.h"
#include "WrappedText.h"

#include <SDL2/SDL.h>

#include "gl_header.h"

using namespace std;

StartConditionsPanel::StartConditionsPanel(PlayerInfo &player, UI &gamePanels, LoadPanel *loadPanel)
	: player(player), gamePanels(gamePanels), loadPanel(loadPanel)
{
	if(GameData::Start().size())
	{
		chosenStart = GameData::Start()[GameData::Start().size()-1];	
		hasChosenStart = true;
	}
	
	descriptionBox = GameData::Interfaces().Get("start conditions menu")->GetBox("start description");
	entryBox = GameData::Interfaces().Get("start conditions menu")->GetBox("start entry");
	entryListBox = GameData::Interfaces().Get("start conditions menu")->GetBox("start entry list");
	entryInternalBox = GameData::Interfaces().Get("start conditions menu")->GetBox("start entry internal");

}


void StartConditionsPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	Information info;

	string descriptionText; // String that will be shown in the description panel

	if(hasChosenStart){
		info.SetCondition("chosen start");
		if(chosenStart.GetSprite())
			info.SetSprite("start sprite", chosenStart.GetSprite());
		info.SetString("name", chosenStart.GetName());
		info.SetString("description", chosenStart.GetDescription());
		info.SetString("planet", chosenStart.GetPlanet()->Name());
		info.SetString("system", chosenStart.GetSystem()->Name());
		info.SetString("date", chosenStart.GetDate().ToString());
		info.SetString("credits", to_string(chosenStart.GetAccounts().Credits()));

		descriptionText = chosenStart.GetDescription();
	}
	else if (!GameData::Start().size())
	{
		descriptionText = "No start scenarios were defined!\n\nMake sure that you installed Endless Sky and all of your plugins properly";
	}
	else
	{
		descriptionText = "";		
	}

	GameData::Background().Draw(Point(), Point());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	GameData::Interfaces().Get("start conditions menu")->Draw(info, this);
	GameData::Interfaces().Get("menu start info")->Draw(info, this);
	
	const Font &font = FontSet::Get(14);

	WrappedText *text = new WrappedText(font);

	text->SetAlignment(WrappedText::LEFT);
	text->SetWrapWidth(210);
	text->Wrap(descriptionText);

	// Only allow draws inside the description box
	glScissor(
		descriptionBox.Left() + GameWindow::TrueWidth() / 2,
		descriptionBox.Top() + GameWindow::TrueHeight() / 2,
		descriptionBox.Width(),
		descriptionBox.Height());

	glEnable(GL_SCISSOR_TEST);
	text->Draw(
		Point(
			descriptionBox.Left(), 
			descriptionBox.Top() + descriptionScroll
		),
		Color(1.,1.,1.));	

	Point point(
		entryListBox.Left(),
		entryListBox.Top() + listScroll 
	);

	// Only draw inside the entry list box
	glScissor(
		0,
		entryListBox.Top() + GameWindow::TrueHeight() / 2,
		GameWindow::TrueWidth(),
		entryListBox.Height()
	);
	for(const auto &it : GameData::Start())
	{
		Rectangle zone(
			point  + Point(
				(entryInternalBox.Width()) / 2, 
				(entryInternalBox.Height()) / 2), 
			entryBox.Dimensions()
		);
		bool isHighlighted = (it == chosenStart);
		
		// double alpha = min(1., max(0., min(.1 * (113. - point.Y()), .1 * (point.Y() - -167.))));
		double alpha = 1;
		
		if(it == chosenStart){
			FillShader::Fill(zone.Center(), zone.Dimensions(), Color(.1 * alpha, 0.));
		}
		
		string name = font.Truncate(it.GetName(), entryInternalBox.Width());
		font.Draw(name, point, Color((isHighlighted ? .7 : .5) * alpha, 0.));
		point += Point(0., entryBox.Height());
	}

	glDisable(GL_SCISSOR_TEST);
}



bool StartConditionsPanel::Drag(double dx, double dy)
{
	int x = hoverPoint.X();
	if(x >= entryListBox.Left() && x < entryListBox.Right())
	{
		// Scroll the list
		// This looks inefficient but it probably gets optimized by the compiler
		listScroll -= dy;
		listScroll = max(-entryBox.Height() * (GameData::Start().size()-1), listScroll); // Avoid people going too low
		listScroll = min(0.,listScroll); // Snap the list to avoid people scrolling too far up
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
	if(key == 'b')
	{
		GetUI()->Pop(this);
		return true;
	}
	else if(key == 'b' || key == SDLK_ESCAPE || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
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
	// The first row of each panel is y = -160 to -140.
	if(y < -160 || y >= (-160 + 14 * 20))
		return false;
	

	if(x >= -470 && x < -250)
	{
		int selected = (y + 0 - -160) / 20;
		int i = 0;
		for(const auto &it : GameData::Start())
			if(i++ == selected)
			{
				if(!(chosenStart == it))
					descriptionScroll = 0; // Reset scrolling
				chosenStart = it;
				hasChosenStart = true;
			}
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
