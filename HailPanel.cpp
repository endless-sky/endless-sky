/* HailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "HailPanel.h"

#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "UI.h"
#include "WrappedText.h"

#include <algorithm>

using namespace std;



HailPanel::HailPanel(PlayerInfo &player, const std::shared_ptr<Ship> &ship)
	: player(player), ship(ship)
{
	const Government *gov = ship->GetGovernment();
	header = gov->GetName() + " ship \"" + ship->Name() + "\":";
	
	if(GameData::GetPolitics().IsEnemy(GameData::PlayerGovernment(), gov))
	{
		// Find the total value of your fleet.
		int value = 0;
		for(const shared_ptr<Ship> &it : player.Ships())
			value += it->Cost();
		
		bribe = static_cast<int>(value * gov->GetBribeFraction() * .001) * 1000;
		if(bribe)
			message = "If you want us to leave you alone, it'll cost you "
				+ Format::Number(bribe) + " credits.";
	}
	else
	{
		// Is the player in any need of assistance?
		const Ship *playerShip = player.GetShip();
		// Check if the player is out of fuel.
		if(!playerShip->JumpsRemaining())
		{
			playerNeedsHelp = true;
			canGiveFuel = ship->CanRefuel(*playerShip);
		}
		// Check if the player is disabled.
		if(playerShip->IsDisabled())
		{
			playerNeedsHelp = true;
			canRepair = true;
		}
		
		if(canGiveFuel || canRepair)
			message = "Looks like you've gotten yourself into a bit of trouble. "
				"Would you like us to ";
		if(canGiveFuel && canRepair)
			message += "patch you up and give you some fuel?";
		else if(canGiveFuel)
			message += "give you some fuel?";
		else if(canRepair)
			message += "patch you up?";
	}
	
	if(message.empty())
		message = ship->GetHail();
}



HailPanel::HailPanel(PlayerInfo &player, const StellarObject *planet)
	: player(player), planet(planet)
{
	const Government *gov = &player.GetSystem()->GetGovernment();
	if(planet->GetPlanet())
		header = gov->GetName() + " planet \"" + planet->GetPlanet()->Name() + "\":";
	
	// TODO: handle bribe requests if the player is not currently allowed to land.
	if(player.GetShip())
		message = "You are cleared to land, " + player.GetShip()->Name() + ".";
}



void HailPanel::Draw() const
{
	DrawBackdrop();
	
	Information interfaceInfo;
	interfaceInfo.SetString("header", header);
	if(ship)
	{
		bool isEnemy = GameData::GetPolitics().IsEnemy(
			GameData::PlayerGovernment(), ship->GetGovernment());
		if(isEnemy)
		{
			interfaceInfo.SetCondition("can bribe");
			interfaceInfo.SetCondition("cannot assist");
		}
		else
			interfaceInfo.SetCondition("can assist");
	}
	else
	{
		bool isEnemy = GameData::GetPolitics().IsEnemy(
			GameData::PlayerGovernment(), &player.GetSystem()->GetGovernment());
		if(isEnemy)
			interfaceInfo.SetCondition("can bribe");
		else
			interfaceInfo.SetCondition("cannot bribe");
		
		interfaceInfo.SetCondition("can dominate");
	}
	
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	interface->Draw(interfaceInfo);
	
	// Draw the sprite, rotated, scaled, and swizzled as necessary.
	int swizzle = ship ? ship->GetGovernment()->GetSwizzle() : 0;
	const Animation &animation = ship ? ship->GetSprite() : planet->GetSprite();
	uint32_t tex = animation.GetSprite()->Texture();
	
	float pos[2] = {-170.f, -10.f};
	
	Point unit = ship ? 2. * ship->Unit() : planet->Position().Unit();
	double zoom = min(1., 200. / max(animation.Width(), animation.Height()));
	Point uw = unit * (animation.Width() * zoom);
	Point uh = unit * (animation.Height() * zoom);
	float tr[4] = {
		static_cast<float>(-uw.Y()), static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()), static_cast<float>(-uh.Y())};
	
	SpriteShader::Bind();
	SpriteShader::Add(tex, tex, pos, tr, swizzle);
	SpriteShader::Unbind();
	
	// Draw the current message.
	WrappedText wrap;
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(330);
	wrap.SetFont(FontSet::Get(14));
	wrap.Wrap(message);
	wrap.Draw(Point(-50., -50.), *GameData::Colors().Get("medium"));
}



bool HailPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	bool shipIsEnemy = ship && GameData::GetPolitics().IsEnemy(
		GameData::PlayerGovernment(), ship->GetGovernment());
	if(key == 'd')
		GetUI()->Pop(this);
	else if(key == 'a' || key == 't' || key == 'h')
	{
		// TODO: handle dominating planets.
		if(planet)
		{
			message = "Please don't joke about that sort of thing.";
			return true;
		}
		else if(shipIsEnemy)
			return false;
		if(playerNeedsHelp)
		{
			if(canGiveFuel || canRepair)
			{
				ship->SetShipToAssist(player.Ships().front());
				message = "Hang on, we'll be there in a minute.";
			}
			else
				message = "Sorry, but if we give you fuel we won't have enough"
					" to make it to the next system.";
		}
		else if(ship)
			message = "You don't seem to be in need of repairs or fuel assistance.";
	}
	else if(key == 'b' || key == 'o')
	{
		// Make sure it actually makes sense to bribe this ship.
		if(ship && !shipIsEnemy)
			return true;
		
		if(bribe)
		{
			if(ship)
				GameData::GetPolitics().Bribe(ship->GetGovernment());
			
			// TODO: handle landing bribes to planets.
			player.Accounts().AddCredits(-bribe);
			message = "It's a pleasure doing business with you.";
			Messages::Add("You bribed a " + ship->GetGovernment()->GetName() + " ship "
				+ Format::Number(bribe) + " credits to refrain from attacking you today.");
			bribe = 0;
		}
		else
			message = "I do not want your money.";
	}
	
	return true;
}



bool HailPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key != '\0')
			return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	return true;
}
