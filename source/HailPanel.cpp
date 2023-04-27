/* HailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "HailPanel.h"

#include "text/alignment.hpp"
#include "DrawList.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "Phrase.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Ship.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"
#include "text/WrappedText.h"

#include <algorithm>
#include <cmath>

using namespace std;



HailPanel::HailPanel(PlayerInfo &player, const shared_ptr<Ship> &ship, function<void(const Government *)> bribeCallback)
	: player(player), ship(ship), bribeCallback(bribeCallback), sprite(ship->GetSprite()), facing(ship->Facing())
{
	SetInterruptible(false);

	const Government *gov = ship->GetGovernment();
	if(!ship->Name().empty())
		header = gov->GetName() + " " + ship->Noun() + " \"" + ship->Name() + "\":";
	else
		header = ship->ModelName() + " (" + gov->GetName() + "): ";
	// Drones are always unpiloted, so they never respond to hails.
	bool isMute = ship->GetPersonality().IsMute() || (ship->Attributes().Category() == "Drone");
	hasLanguage = !isMute && (gov->Language().empty() || player.Conditions().Get("language: " + gov->Language()));
	canAssistPlayer = !ship->CanBeCarried();

	if(isMute)
		message = "(There is no response to your hail.)";
	else if(!hasLanguage)
		message = "(An alien voice says something in a language you do not recognize.)";
	else if(gov->IsEnemy())
	{
		// Enemy ships always show hostile messages.
		// They either show bribing messages,
		// or standard hostile messages, if disabled.
		if(!ship->IsDisabled())
			SetBribe(gov->GetBribeFraction());
	}
	else if(ship->IsDisabled())
	{
		const Ship *flagship = player.Flagship();
		if(flagship->NeedsFuel(false) || flagship->IsDisabled())
			message = "Sorry, we can't help you, because our ship is disabled.";
	}
	else
	{
		// Is the player in any need of assistance?
		const Ship *flagship = player.Flagship();
		// Check if the player is out of fuel.
		if(flagship->NeedsFuel(false))
		{
			playerNeedsHelp = true;
			canGiveFuel = ship->CanRefuel(*flagship) && canAssistPlayer;
		}
		// Check if the player is disabled.
		if(flagship->IsDisabled())
		{
			playerNeedsHelp = true;
			canRepair = canAssistPlayer;
		}

		if(ship->GetPersonality().IsSurveillance())
		{
			canGiveFuel = false;
			canRepair = false;
		}

		if(ship->GetShipToAssist() == player.FlagshipPtr())
			message = "Hang on, we'll be there in a minute.";
		else if(canGiveFuel || canRepair)
		{
			message = "Looks like you've gotten yourself into a bit of trouble. "
				"Would you like us to ";
			if(canGiveFuel && canRepair)
				message += "patch you up and give you some fuel?";
			else if(canGiveFuel)
				message += "give you some fuel?";
			else if(canRepair)
				message += "patch you up?";
		}
		else if(playerNeedsHelp && !canAssistPlayer)
			message = "Sorry, my ship is too small to have the right equipment to assist you.";
	}

	if(message.empty())
		message = ship->GetHail(player.GetSubstitutions());
}



HailPanel::HailPanel(PlayerInfo &player, const StellarObject *object)
	: player(player), planet(object->GetPlanet()), sprite(object->GetSprite()), facing(object->Facing())
{
	SetInterruptible(false);

	const Government *gov = planet ? planet->GetGovernment() : player.GetSystem()->GetGovernment();
	if(planet)
		header = gov->GetName() + " " + planet->Noun() + " \"" + planet->Name() + "\":";
	hasLanguage = (gov->Language().empty() || player.Conditions().Get("language: " + gov->Language()));

	// If the player is hailing a planet, determine if a mission grants them clearance before checking
	// if they have a language that matches the planet's government. This allows mission clearance
	// to bypass language barriers.
	if(planet && player.Flagship())
		for(const Mission &mission : player.Missions())
			if(mission.HasClearance(planet) && mission.ClearanceMessage() != "auto"
					&& mission.HasFullClearance())
			{
				planet->Bribe();
				message = mission.ClearanceMessage();
				return;
			}

	if(!hasLanguage)
		message = "(An alien voice says something in a language you do not recognize.)";
	else if(planet && player.Flagship())
	{
		if(planet->CanLand())
			message = "You are cleared to land, " + player.Flagship()->Name() + ".";
		else
		{
			SetBribe(planet->GetBribeFraction());
			if(bribe)
				message = "If you want to land here, it'll cost you "
					+ Format::CreditString(bribe) + ".";
			else if(gov->IsEnemy())
				message = "You are not welcome here.";
			else
				message = "I'm afraid we can't permit you to land here.";
		}
	}
}



void HailPanel::Draw()
{
	DrawBackdrop();

	Information info;
	info.SetString("header", header);
	if(ship)
	{
		info.SetCondition("show assist");
		if(hasLanguage && !ship->IsDisabled())
		{
			if(requestedToBribeShip)
				info.SetCondition("show pay bribe");
			if(ship->GetGovernment()->IsEnemy())
			{
				if(requestedToBribeShip)
					info.SetCondition("can pay bribe");
				else
					info.SetCondition("can bribe");
			}
			else if(!ship->CanBeCarried() && ship->GetShipToAssist() != player.FlagshipPtr())
				info.SetCondition("can assist");
		}
	}
	else
	{
		if(!GameData::GetPolitics().HasDominated(planet))
			info.SetCondition("show dominate");
		else
			info.SetCondition("show relinquish");
		if(hasLanguage)
		{
			info.SetCondition("can dominate");
			if(!planet->CanLand())
				info.SetCondition("can bribe");
		}
	}

	const Interface *hailUi = GameData::Interfaces().Get("hail panel");
	hailUi->Draw(info, this);

	// Draw the sprite, rotated, scaled, and swizzled as necessary.
	float zoom = min(2.f, 400.f / max(sprite->Width(), sprite->Height()));
	Point center(-170., -10.);

	DrawList draw;
	// If this is a ship, copy its swizzle, animation settings, etc.
	if(ship)
		draw.Add(Body(*ship, center, Point(), facing, zoom));
	else
		draw.Add(Body(sprite, center, Point(), facing, zoom));

	// If hailing a ship, draw its turret sprites.
	if(ship)
		for(const Hardpoint &hardpoint : ship->Weapons())
			if(hardpoint.GetOutfit() && hardpoint.GetOutfit()->HardpointSprite().HasSprite())
			{
				Body body(
					hardpoint.GetOutfit()->HardpointSprite(),
					center + zoom * facing.Rotate(hardpoint.GetPoint()),
					Point(),
					facing + hardpoint.GetAngle(),
					zoom);
				draw.Add(body);
			}
	draw.Draw();

	// Draw the current message.
	WrappedText wrap;
	wrap.SetAlignment(Alignment::JUSTIFIED);
	wrap.SetWrapWidth(330);
	wrap.SetFont(FontSet::Get(14));
	wrap.Wrap(message);
	wrap.Draw(Point(-50., -50.), *GameData::Colors().Get("medium"));
}



bool HailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool shipIsEnemy = (ship && ship->GetGovernment()->IsEnemy());

	if(key == 'd' || key == SDLK_ESCAPE || key == SDLK_RETURN || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
	{
		if(bribeCallback && bribed)
			bribeCallback(bribed);
		GetUI()->Pop(this);
	}
	else if(key == 't' && hasLanguage && planet)
	{
		if(GameData::GetPolitics().HasDominated(planet))
		{
			GameData::GetPolitics().DominatePlanet(planet, false);
			// Set payment 0 to erase the tribute.
			player.SetTribute(planet, 0);
			message = "Thank you for granting us our freedom!";
		}
		else
			message = planet->DemandTribute(player);
		return true;
	}
	else if(key == 'h' && hasLanguage && ship && canAssistPlayer)
	{
		if(shipIsEnemy || ship->IsDisabled())
			return false;
		if(playerNeedsHelp)
		{
			if(ship->GetPersonality().IsSurveillance())
				message = "Sorry, I'm too busy to help you right now.";
			else if(canGiveFuel || canRepair)
			{
				ship->SetShipToAssist(player.FlagshipPtr());
				message = "Hang on, we'll be there in a minute.";
			}
			else if(ship->Fuel())
				message = "Sorry, but if we give you fuel we won't have enough to make it to the next system.";
			else
				message = "Sorry, we don't have any fuel.";
		}
		else
		{
			if(bribe)
				message = "Yeah, right. Don't push your luck.";
			else
				message = "You don't seem to be in need of repairs or fuel assistance.";
		}
	}
	else if((key == 'b' || key == 'o') && hasLanguage)
	{
		// Make sure it actually makes sense to bribe this ship.
		if((ship && !shipIsEnemy) || (planet && planet->CanLand()))
			return true;

		if(bribe > player.Accounts().Credits())
			message = "Sorry, but you don't have enough money to be worth my while.";
		else if(bribe)
		{
			if(!ship || requestedToBribeShip)
			{
				player.Accounts().AddCredits(-bribe);
				message = "It's a pleasure doing business with you.";
			}
			if(ship)
			{
				if(!requestedToBribeShip)
				{
					message = "If you want us to leave you alone, it'll cost you "
						+ Format::CreditString(bribe) + ".";
					requestedToBribeShip = true;
				}
				else
				{
					bribed = ship->GetGovernment();
					bribed->Bribe();
					Messages::Add("You bribed a " + bribed->GetName() + " ship "
						+ Format::CreditString(bribe) + " to refrain from attacking you today."
							, Messages::Importance::High);
				}
			}
			else
			{
				planet->Bribe();
				Messages::Add("You bribed the authorities on " + planet->Name() + " "
					+ Format::CreditString(bribe) + " to permit you to land."
						, Messages::Importance::High);
			}
		}
		else
			message = "I do not want your money.";
	}

	return true;
}



void HailPanel::SetBribe(double scale)
{
	// Find the total value of your fleet.
	int64_t value = 0;
	for(const shared_ptr<Ship> &it : player.Ships())
		value += it->Cost();

	bribe = 1000 * static_cast<int64_t>(sqrt(value) * scale);
	if(scale && !bribe)
		bribe = 1000;
}
