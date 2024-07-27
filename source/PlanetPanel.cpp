/* PlanetPanel.cpp
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

#include "PlanetPanel.h"

#include "Information.h"

#include "text/alignment.hpp"
#include "BankPanel.h"
#include "Command.h"
#include "ConversationPanel.h"
#include "Dialog.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "HiringPanel.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "MessageLogPanel.h"
#include "MissionPanel.h"
#include "OutfitterPanel.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PlayerInfoPanel.h"
#include "Port.h"
#include "Ship.h"
#include "ShipyardPanel.h"
#include "SpaceportPanel.h"
#include "System.h"
#include "TaskQueue.h"
#include "TradingPanel.h"
#include "UI.h"

#include <sstream>

using namespace std;



PlanetPanel::PlanetPanel(PlayerInfo &player, function<void()> callback)
	: player(player), callback(callback),
	planet(*player.GetPlanet()), system(*player.GetSystem()),
	ui(*GameData::Interfaces().Get("planet"))
{
	trading.reset(new TradingPanel(player));
	bank.reset(new BankPanel(player));
	spaceport.reset(new SpaceportPanel(player));
	hiring.reset(new HiringPanel(player));

	text.SetFont(FontSet::Get(14));
	text.SetAlignment(Alignment::JUSTIFIED);
	text.SetWrapWidth(480);

	// Since the loading of landscape images is deferred, make sure that the
	// landscapes for this system are loaded before showing the planet panel.
	TaskQueue queue;
	GameData::Preload(queue, planet.Landscape());
	queue.Wait();
	queue.ProcessSyncTasks();
}



void PlanetPanel::Step()
{
	// If the player is dead, pop the planet panel.
	if(player.IsDead())
	{
		player.SetPlanet(nullptr);
		GetUI()->PopThrough(this);
		return;
	}

	// If the previous mission callback resulted in a "launch", take off now.
	const Ship *flagship = player.Flagship();
	if(flagship && flagship->CanBeFlagship() && (player.ShouldLaunch() || requestedLaunch))
	{
		TakeOffIfReady();
		return;
	}

	// Handle missions for locations that aren't handled separately,
	// treating them all as the landing location. This is mainly to
	// handle the intro mission in the event the player moves away
	// from the landing before buying a ship.
	const Panel *activePanel = selectedPanel ? selectedPanel : this;
	if(activePanel != spaceport.get() && GetUI()->IsTop(activePanel))
	{
		Mission *mission = player.MissionToOffer(Mission::LANDING);
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		else
			player.HandleBlockedMissions(Mission::LANDING, GetUI());
	}
}



void PlanetPanel::Draw()
{
	Information info;
	info.SetSprite("land", planet.Landscape());

	const Ship *flagship = player.Flagship();
	if(flagship && flagship->CanBeFlagship())
		info.SetCondition("has ship");

	if(planet.CanUseServices())
	{
		const Port &port = planet.GetPort();

		if(port.HasService(Port::ServicesType::Bank))
			info.SetCondition("has bank");
		if(port.HasService(Port::ServicesType::JobBoard))
			info.SetCondition("has job board");
		if(port.HasService(Port::ServicesType::HireCrew))
			info.SetCondition("can hire crew");
		if(port.HasService(Port::ServicesType::Trading) && system.HasTrade())
			info.SetCondition("has trade");
		if(planet.HasNamedPort())
		{
			info.SetCondition("has port");
			info.SetString("port name", port.Name());
		}

		if(planet.HasShipyard())
			info.SetCondition("has shipyard");

		if(planet.HasOutfitter())
			info.SetCondition("has outfitter");
	}

	ui.Draw(info, this);

	if(!selectedPanel)
	{
		Rectangle box = ui.GetBox("content");
		if(box.Width() != text.WrapWidth())
			text.SetWrapWidth(box.Width());
		text.Wrap(planet.Description().ToString(player.Conditions()));
		text.Draw(box.TopLeft(), *GameData::Colors().Get("bright"));
	}
}



// Only override the ones you need; the default action is to return false.
bool PlanetPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(player.IsDead())
		return true;

	Panel *oldPanel = selectedPanel;
	const Ship *flagship = player.Flagship();

	bool hasAccess = planet.CanUseServices();
	if(key == 'd' && flagship && flagship->CanBeFlagship())
	{
		requestedLaunch = true;
		return true;
	}
	else if(key == 'l')
		selectedPanel = nullptr;
	else if(key == 't' && hasAccess
			&& planet.GetPort().HasService(Port::ServicesType::Trading) && system.HasTrade())
	{
		selectedPanel = trading.get();
		GetUI()->Push(trading);
	}
	else if(key == 'b' && hasAccess && planet.GetPort().HasService(Port::ServicesType::Bank))
	{
		selectedPanel = bank.get();
		GetUI()->Push(bank);
	}
	else if(key == 'p' && hasAccess && planet.HasNamedPort())
	{
		selectedPanel = spaceport.get();
		if(isNewPress)
			spaceport->UpdateNews();
		GetUI()->Push(spaceport);
	}
	else if(key == 's' && hasAccess && planet.HasShipyard())
	{
		GetUI()->Push(new ShipyardPanel(player));
		return true;
	}
	else if(key == 'o' && hasAccess && planet.HasOutfitter())
	{
		GetUI()->Push(new OutfitterPanel(player));
		return true;
	}
	else if(key == 'j' && hasAccess && planet.GetPort().HasService(Port::ServicesType::JobBoard))
	{
		GetUI()->Push(new MissionPanel(player));
		return true;
	}
	else if(key == 'h' && hasAccess && planet.GetPort().HasService(Port::ServicesType::HireCrew))
	{
		selectedPanel = hiring.get();
		GetUI()->Push(hiring);
	}
	else if(command.Has(Command::MAP))
	{
		GetUI()->Push(new MapDetailPanel(player));
		return true;
	}
	else if(command.Has(Command::INFO))
	{
		GetUI()->Push(new PlayerInfoPanel(player));
		return true;
	}
	else if(command.Has(Command::MESSAGE_LOG))
	{
		GetUI()->Push(new MessageLogPanel());
		return true;
	}
	else
		return false;

	// If we are here, it is because something happened to change the selected
	// planet UI panel. So, we need to pop the old selected panel:
	if(oldPanel)
		GetUI()->Pop(oldPanel);

	return true;
}



void PlanetPanel::TakeOffIfReady()
{
	// If we're currently showing a conversation or dialog, wait for it to close.
	if(!GetUI()->IsTop(this) && !GetUI()->IsTop(trading.get()) && !GetUI()->IsTop(bank.get())
			&& !GetUI()->IsTop(spaceport.get()) && !GetUI()->IsTop(hiring.get()))
		return;

	// If something happens here that cancels the order to take off, don't try
	// to take off until the button is clicked again.
	requestedLaunch = false;

	absentCannotFly.clear();

	// Check for any landing missions that have not been offered.
	Mission *mission = player.MissionToOffer(Mission::LANDING);
	if(mission)
	{
		mission->Do(Mission::OFFER, player, GetUI());
		return;
	}

	// Check whether the player can be warned before takeoff.
	if(player.ShouldLaunch())
	{
		TakeOff(true);
		return;
	}

	// Check if any of the player's ships are configured in such a way that they
	// will be impossible to fly. If so, let the player choose whether to park them.
	ostringstream out;
	flightChecks = player.FlightCheck();
	if(!flightChecks.empty())
	{
		for(const auto &result : flightChecks)
		{
			// If there is a flight check error, it will be the first (and only) entry.
			auto &check = result.second.front();
			if(check.back() == '!')
			{
				// If the ship with a flight check error is in another system, then the only thing the player
				// can do is park it. But if the ship is with the player, then they may be able to make changes
				// to rectify the error. As such, provide a conversation for any single present ship, but
				// record and report all absent ships later.
				if(result.first->GetSystem() != &system)
				{
					out << result.first->Name() << ", ";
					absentCannotFly.push_back(result.first);
				}
				else
				{
					GetUI()->Push(new ConversationPanel(player,
						*GameData::Conversations().Get("flight check: " + check), nullptr, nullptr, result.first));
					return;
				}
			}
		}
		if(!absentCannotFly.empty())
		{
			string shipNames = out.str();
			// Pop back the last ", " in the string.
			shipNames.pop_back();
			shipNames.pop_back();
			GetUI()->Push(new Dialog(this, &PlanetPanel::CheckWarningsAndTakeOff,
				"Some of your ships in other systems are not be able to fly:\n" + shipNames +
				"\nDo you want to park those ships and depart?", Truncate::MIDDLE));
			return;
		}
	}

	CheckWarningsAndTakeOff();
}



void PlanetPanel::CheckWarningsAndTakeOff()
{
	// Park out of system ships that cannot fly.
	for(const auto &ship : absentCannotFly)
		ship->SetIsParked(true);
	absentCannotFly.clear();

	// Check for items that would be sold, or mission passengers that would be abandoned on-planet.
	const Ship *flagship = player.Flagship();
	const CargoHold &cargo = player.DistributeCargo();
	// Are you overbooked? Don't count fireable flagship crew.
	// (If your ship can't support its required crew, it is counted as having no fireable crew.)
	const int overbooked = cargo.Passengers() - max(0, flagship->Crew() - flagship->RequiredCrew());
	const int missionCargoToSell = cargo.MissionCargoSize();
	// Will you have to sell something other than regular cargo?
	const int commoditiesToSell = cargo.CommoditiesSize();
	int outfitsToSell = 0;
	map<const Outfit *, int> uniquesToSell;
	for(auto &it : cargo.Outfits())
	{
		outfitsToSell += it.second;
		if(it.first->Attributes().Get("unique"))
			uniquesToSell[it.first] = it.second;
	}
	// Have you left any unique items at the outfitter?
	map<const Outfit *, int> leftUniques;
	for(const auto &it : player.GetStock())
		if(it.second > 0 && it.first->Attributes().Get("unique"))
			leftUniques[it.first] = it.second;
	// Count how many active ships we have that cannot make the jump (e.g. due to lack of fuel,
	// drive, or carrier). All such ships will have been logged in the player's flightcheck.
	size_t nonJumpCount = 0;
	if(!flightChecks.empty())
	{
		// There may be multiple warnings reported, but only 3 result in a ship which cannot jump.
		const auto jumpWarnings = set<string>{
			"no bays?", "no fuel?", "no hyperdrive?"
		};
		for(const auto &result : flightChecks)
			for(const auto &warning : result.second)
				if(jumpWarnings.count(warning))
				{
					++nonJumpCount;
					break;
				}
	}

	if(nonJumpCount > 0 || missionCargoToSell > 0 || outfitsToSell > 0 || commoditiesToSell > 0 || overbooked > 0
		|| !leftUniques.empty())
	{
		ostringstream out;
		auto ListUniques = [&out] (const map<const Outfit *, int> &uniques)
		{
			const int detailedSize = (uniques.size() > 5 ? 4 : uniques.size());
			auto it = uniques.begin();
			for(int i = 0; i < detailedSize; ++i)
			{
				out << "\n" + to_string(it->second) + " "
					+ (it->second == 1 ? it->first->DisplayName() : it->first->PluralName());
				++it;
			}
			int otherUniquesCount = 0;
			if(it != uniques.end())
			{
				for( ; it != uniques.end(); ++it)
					otherUniquesCount += it->second;
				out << "\nand " + to_string(otherUniquesCount) + " other unique outfits.";
			}
			else
				out << ".";
		};
		out << "If you take off now, you will:";

		// Warn about missions that will fail on takeoff.
		if(missionCargoToSell > 0 || overbooked > 0)
		{
			out << "\n- abort a mission due to not having enough ";

			if(overbooked > 0)
			{
				out << "bunks available for " << overbooked;
				out << (overbooked > 1 ? " of the passengers" : " passenger");
				out << (missionCargoToSell > 0 ? " and not having enough " : ".");
			}

			if(missionCargoToSell > 0)
				out << "cargo space to hold " << Format::CargoString(missionCargoToSell, "mission cargo.");
		}
		// Warn about outfits that can't be carried.
		if(outfitsToSell > 0)
		{
			out << "\n- ";
			out << (planet.HasOutfitter() ? "store " : "sell ") << outfitsToSell << " outfit";
			out << (outfitsToSell > 1 ? "s" : "");
			out << " that none of your ships can hold.";
			if(!uniquesToSell.empty())
			{
				out << " Some of the outfits are unique:";
				ListUniques(uniquesToSell);
			}
		}
		// Warn about unique items you sold.
		if(!leftUniques.empty())
		{
			out << "\n- not be able to re-purchase unique outfits you sold at the outfitter:";
			ListUniques(leftUniques);
		}
		// Warn about ships that won't travel with you.
		if(nonJumpCount > 0)
		{
			out << "\n- launch with ";
			if(nonJumpCount == 1)
				out << "a ship";
			else
				out << nonJumpCount << " ships";
			out << " that will not be able to leave the system.";
		}
		// Warn about commodities you will have to sell.
		if(commoditiesToSell > 0)
		{
			out << "\n- sell " << Format::CargoString(commoditiesToSell, "cargo");
			out << " that you do not have space for.";
		}
		out << "\nAre you sure you want to continue?";
		GetUI()->Push(new Dialog(this, &PlanetPanel::WarningsDialogCallback, out.str()));
		return;
	}

	// There was no need to ask the player whether we can get rid of anything,
	// so go ahead and take off.
	TakeOff(false);
}



void PlanetPanel::WarningsDialogCallback(const bool isOk)
{
	if(isOk)
		TakeOff(false);
	else
		player.PoolCargo();
}



void PlanetPanel::TakeOff(const bool distributeCargo)
{
	flightChecks.clear();
	player.Save();
	if(player.TakeOff(GetUI(), distributeCargo))
	{
		if(callback)
			callback();
		if(selectedPanel)
			GetUI()->Pop(selectedPanel);
		GetUI()->Pop(this);
	}
}
