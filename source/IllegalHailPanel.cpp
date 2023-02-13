/* IllegalHailPanel.cpp
Copyright (c) 2021 by quyykk

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "IllegalHailPanel.h"

#include "text/alignment.hpp"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Politics.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "Sprite.h"
#include "UI.h"

#include <algorithm>
#include <cmath>

using namespace std;



IllegalHailPanel::IllegalHailPanel(PlayerInfo &player, const Ship &hailingShip, Ship &scannedShip,
		const Politics::Punishment &fine)
	: player(player), hailingShip(hailingShip), scannedShip(scannedShip), fine(fine)
{
	const Government *gov = hailingShip.GetGovernment();
	if(!hailingShip.Name().empty())
		header = gov->GetName() + " " + hailingShip.Noun() + " \"" + hailingShip.Name() + "\"";
	else
		header = hailingShip.ModelName() + " (" + gov->GetName() + ")";
	header += " is hailing you:";

	static const std::string defaultMessage
		= "You've been detected carrying illegal <type> and have been issued a fine of <fine>."
			" \n\tDump your cargo immediately or we'll be forced to disable and board your ship.";


	cantSurrender = fine.reason == Politics::Punishment::Outfit;

	map<string, string> subs = {
		{"<type>", fine.reason & Politics::Punishment::Outfit ? "outfits" : "cargo"},
		{"<fine>", Format::CreditString(fine.cost)},
	};
	message =
		Format::Replace(gov->GetInterdiction().empty() ? defaultMessage : gov->GetInterdiction(), subs);

	if(gov->GetBribeFactor())
	{
		static const std::string defaultBribe
			= "If you want us to leave you alone, it'll cost you <bribe>.";

		bribe = static_cast<int64_t>(fine.cost * gov->GetBribeFactor());
		subs["<bribe>"] = Format::CreditString(bribe);

		auto bribeMessage =
			Format::Replace(gov->GetInterdictionBribe().empty() ? defaultBribe : gov->GetInterdictionBribe(),
					subs);
		message += "\n\t";
		message += bribeMessage;
	}
}



void IllegalHailPanel::Draw()
{
	DrawHail();
	DrawIcon(hailingShip);

	Information info;
	if(bribe)
		info.SetCondition("can bribe");
	info.SetCondition(cantSurrender ? "can pay" : "can surrender");

	const Interface *hailUi = GameData::Interfaces().Get("illegal hail panel");
	hailUi->Draw(info, this);
}



bool IllegalHailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(((key == 's' || key == 'c') && !cantSurrender)
			|| ((key == 'p' || key == 'f') && cantSurrender))
	{
		// Dump illegal cago. Only spare outfits are removed. Any mission cargo
		// is automatically removed since the missions are failed.
		for(const auto &pair : scannedShip.Cargo().Outfits())
			if(pair.first->Get("illegal") || pair.first->Get("atrocity") > 0.)
				scannedShip.Jettison(pair.first, pair.second, true);

		// Pay the required fine.
		player.Accounts().AddFine(fine.cost);
		GetUI()->Pop(this);
	}
	else if(key == 'f')
	{
		hailingShip.GetGovernment()->Offend(ShipEvent::PROVOKE);
		GetUI()->Pop(this);
	}
	else if((key == 'o' || key == 'b') && bribe)
	{
		if(bribe > player.Accounts().Credits())
		{
			message = "Sorry, but you don't have enough money to be worth my while.";
			return true;
		}

		player.Accounts().AddCredits(-bribe);
		Messages::Add("You bribed a " + hailingShip.GetGovernment()->GetName() + " ship "
			+ Format::CreditString(bribe) + " to avoid paying a fine today."
				, Messages::Importance::High);

		GetUI()->Pop(this);
	}

	return true;
}
