/* AI.cpp
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

#include "AI.h"

#include "audio/Audio.h"
#include "Command.h"
#include "ConditionsStore.h"
#include "DistanceMap.h"
#include "FighterHitHelper.h"
#include "Flotsam.h"
#include "text/Format.h"
#include "FormationPattern.h"
#include "FormationPositioner.h"
#include "GameData.h"
#include "Gamerules.h"
#include "Government.h"
#include "Hardpoint.h"
#include "JumpType.h"
#include "image/Mask.h"
#include "Messages.h"
#include "Minable.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Port.h"
#include "Preferences.h"
#include "Random.h"
#include "RoutePlan.h"
#include "Ship.h"
#include "ship/ShipAICache.h"
#include "ShipEvent.h"
#include "ShipJumpNavigation.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"
#include "Weapon.h"
#include "Wormhole.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <utility>

using namespace std;

namespace {
	// If the player issues any of those commands, then any autopilot actions for the player get cancelled.
	const Command &AutopilotCancelCommands()
	{
		static const Command cancelers(Command::LAND | Command::JUMP | Command::FLEET_JUMP | Command::BOARD
			| Command::AFTERBURNER | Command::BACK | Command::FORWARD | Command::LEFT | Command::RIGHT
			| Command::AUTOSTEER | Command::STOP);

		return cancelers;
	}

	bool NeedsFuel(const Ship &ship)
	{
		return ship.GetSystem() && !ship.IsEnteringHyperspace() && !ship.GetSystem()->HasFuelFor(ship)
				&& ship.NeedsFuel();
	}

	bool NeedsEnergy(const Ship &ship)
	{
		return ship.GetSystem() && !ship.IsEnteringHyperspace() && ship.NeedsEnergy();
	}

	bool IsStranded(const Ship &ship)
	{
		return NeedsFuel(ship) || NeedsEnergy(ship);
	}

	bool CanBoard(const Ship &ship, const Ship &target)
	{
		if(&ship == &target)
			return false;
		if(target.IsDestroyed() || !target.IsTargetable() || target.GetSystem() != ship.GetSystem())
			return false;
		if(IsStranded(target) && !ship.GetGovernment()->IsEnemy(target.GetGovernment()))
			return true;
		return target.IsDisabled();
	}

	// Check if the given ship can "swarm" the targeted ship, e.g. to provide anti-missile cover.
	bool CanSwarm(const Ship &ship, const Ship &target)
	{
		if(target.GetPersonality().IsSwarming() || target.IsHyperspacing())
			return false;
		if(target.GetGovernment()->IsEnemy(ship.GetGovernment()))
			return false;
		if(target.GetSystem() != ship.GetSystem())
			return false;
		return target.IsTargetable();
	}

	double AngleDiff(double a, double b)
	{
		a = abs(a - b);
		return min(a, 360. - a);
	}

	// Determine if all able, non-carried escorts are ready to jump with this
	// ship. Carried escorts are waited for in AI::Step.
	bool EscortsReadyToJump(const Ship &ship)
	{
		bool shipIsYours = ship.IsYours();
		const Government *gov = ship.GetGovernment();
		for(const weak_ptr<Ship> &ptr : ship.GetEscorts())
		{
			shared_ptr<const Ship> escort = ptr.lock();
			// Skip escorts which are not player-owned and not escort mission NPCs.
			if(!escort || (shipIsYours && !escort->IsYours() && (!escort->GetPersonality().IsEscort()
					|| gov->IsEnemy(escort->GetGovernment()))))
				continue;
			if(!escort->IsDisabled() && !escort->CanBeCarried()
					&& escort->GetSystem() == ship.GetSystem()
					&& escort->JumpNavigation().JumpFuel() && !escort->IsReadyToJump(true))
				return false;
		}
		return true;
	}

	bool EscortsReadyToLand(const Ship &ship)
	{
		bool shipIsYours = ship.IsYours();
		const Government *gov = ship.GetGovernment();
		for(const weak_ptr<Ship> &ptr : ship.GetEscorts())
		{
			shared_ptr<const Ship> escort = ptr.lock();
			// Skip escorts which are not player-owned and not escort mission NPCs.
			if(!escort || (shipIsYours && !escort->IsYours() && (!escort->GetPersonality().IsEscort()
				|| gov->IsEnemy(escort->GetGovernment()))))
				continue;
			if(escort->IsDisabled())
				continue;
			if(escort->GetTargetStellar() == ship.GetTargetStellar() && !escort->CanLand())
				return false;
		}
		return true;
	}

	// Determine if the ship has any usable weapons.
	bool IsArmed(const Ship &ship)
	{
		for(const Hardpoint &hardpoint : ship.Weapons())
		{
			const Weapon *weapon = hardpoint.GetWeapon();
			if(weapon && !hardpoint.IsSpecial())
			{
				const Outfit *ammo = weapon->Ammo();
				if(ammo && !ship.OutfitCount(ammo))
					continue;
				return true;
			}
		}
		return false;
	}

	void Deploy(const Ship &ship, bool includingDamaged)
	{
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.ship && (includingDamaged || bay.ship->Health() > .75) &&
					(!bay.ship->IsYours() || bay.ship->HasDeployOrder()))
				bay.ship->SetCommands(Command::DEPLOY);
	}

	// Helper function for selecting the ships for formation commands.
	vector<Ship *> GetShipsForFormationCommand(const PlayerInfo &player)
	{
		// Figure out what ships we are giving orders to.
		vector<Ship *> targetShips;
		auto &selectedShips = player.SelectedEscorts();
		// If selectedShips is empty, this applies to the whole fleet.
		if(selectedShips.empty())
		{
			auto &playerShips = player.Ships();
			targetShips.reserve(playerShips.size() - 1);
			for(const shared_ptr<Ship> &it : player.Ships())
				if(it.get() != player.Flagship() && !it->IsParked()
						&& !it->IsDestroyed() && !it->IsDisabled())
					targetShips.push_back(it.get());
		}
		else
		{
			targetShips.reserve(selectedShips.size());
			for(const weak_ptr<Ship> &it : selectedShips)
			{
				shared_ptr<Ship> ship = it.lock();
				if(ship)
					targetShips.push_back(ship.get());
			}
		}

		return targetShips;
	}

	// Issue deploy orders for the selected ships (or the full fleet if no ships are selected).
	void IssueDeploy(const PlayerInfo &player)
	{
		// Lay out the rules for what constitutes a deployable ship. (Since player ships are not
		// deleted from memory until the next landing, check both parked and destroyed states.)
		auto isCandidate = [](const shared_ptr<Ship> &ship) -> bool
		{
			return ship->CanBeCarried() && !ship->IsParked() && !ship->IsDestroyed();
		};

		auto toDeploy = vector<Ship *> {};
		auto toRecall = vector<Ship *> {};
		{
			auto maxCount = player.Ships().size() / 2;
			toDeploy.reserve(maxCount);
			toRecall.reserve(maxCount);
		}

		// First, check if the player selected any carried ships.
		for(const weak_ptr<Ship> &it : player.SelectedEscorts())
		{
			shared_ptr<Ship> ship = it.lock();
			if(ship && isCandidate(ship))
				(ship->HasDeployOrder() ? toRecall : toDeploy).emplace_back(ship.get());
		}
		// If needed, check the player's fleet for deployable ships.
		if(toDeploy.empty() && toRecall.empty())
			for(const shared_ptr<Ship> &ship : player.Ships())
				if(isCandidate(ship) && ship.get() != player.Flagship())
					(ship->HasDeployOrder() ? toRecall : toDeploy).emplace_back(ship.get());

		// If any ships were not yet ordered to deploy, deploy them.
		if(!toDeploy.empty())
		{
			for(Ship *ship : toDeploy)
				ship->SetDeployOrder(true);
			string ship = (toDeploy.size() == 1 ? "ship" : "ships");
			Messages::Add({"Deployed " + to_string(toDeploy.size()) + " carried " + ship + ".",
				GameData::MessageCategories().Get("normal")});
		}
		// Otherwise, instruct the carried ships to return to their berth.
		else if(!toRecall.empty())
		{
			for(Ship *ship : toRecall)
				ship->SetDeployOrder(false);
			string ship = (toRecall.size() == 1 ? "ship" : "ships");
			Messages::Add({"Recalled " + to_string(toRecall.size()) + " carried " + ship + ".",
				GameData::MessageCategories().Get("normal")});
		}
	}

	// Check if the ship contains a carried ship that needs to launch.
	bool HasDeployments(const Ship &ship)
	{
		for(const Ship::Bay &bay : ship.Bays())
			if(bay.ship && bay.ship->HasDeployOrder())
				return true;
		return false;
	}

	// Determine if the ship with the given travel plan should refuel in
	// its current system, or if it should keep traveling.
	bool ShouldRefuel(const Ship &ship, const RoutePlan &route)
	{
		// If we can't get to the destination --- no reason to refuel.
		// (though AI may choose to elsewhere)
		if(!route.HasRoute())
			return false;

		// If the ship is full, no refuel.
		if(ship.Fuel() == 1.)
			return false;

		// If the ship has nowhere to refuel, no refuel.
		const System *from = ship.GetSystem();
		if(!from->HasFuelFor(ship))
			return false;

		// If the ship doesn't have fuel, no refuel.
		double fuelCapacity = ship.Attributes().Get("fuel capacity");
		if(!fuelCapacity)
			return false;

		// If the ship has no drive (or doesn't require fuel), no refuel.
		if(!ship.JumpNavigation().JumpFuel())
			return false;

		// Now we know it could refuel. But it could also jump along the route
		// and refuel later. Calculate if it can reach the next refuel.
		double fuel = fuelCapacity * ship.Fuel();
		const vector<pair<const System *, int>> costs = route.FuelCosts();
		for(auto it = costs.rbegin(); it != costs.rend(); ++it)
		{
			// If the next system with fuel is outside the range of this ship, should refuel.
			if(it->first->HasFuelFor(ship))
				return fuel < it->second;
		}

		// If no system on the way has fuel, refuel if needed to get to the destination.
		return fuel < route.RequiredFuel();
	}

	// The health remaining before becoming disabled, at which fighters and
	// other ships consider retreating from battle.
	const double RETREAT_HEALTH = .25;

	bool ShouldFlee(Ship &ship)
	{
		const Personality &personality = ship.GetPersonality();

		if(personality.IsFleeing())
			return true;

		if(personality.IsStaying())
			return false;

		const bool lowHealth = ship.Health() < RETREAT_HEALTH + .25 * personality.IsCoward();
		if(!personality.IsDaring() && lowHealth)
			return true;

		if(ship.GetAICache().NeedsAmmo())
			return true;

		if(personality.IsGetaway() && ship.Cargo().Free() == 0 && !ship.GetParent())
			return true;

		return false;
	}

	bool StayOrLinger(Ship &ship)
	{
		const Personality &personality = ship.GetPersonality();
		if(personality.IsStaying())
			return true;

		const Government *government = ship.GetGovernment();
		shared_ptr<const Ship> parent = ship.GetParent();
		if(parent && government && parent->GetGovernment()->IsEnemy(government))
			return true;

		if(ship.IsFleeing())
			return false;

		// Everything after here is the lingering logic.

		if(!personality.IsLingering())
			return false;
		// NPCs that can follow the player do not linger.
		if(ship.IsSpecial() && !personality.IsUninterested())
			return false;

		const System *system = ship.GetSystem();
		const System *destination = ship.GetTargetSystem();

		// Ships not yet at their destination must go there before they can linger.
		if(destination && destination != system)
			return false;
		// Ship cannot linger any longer in this system.
		if(!system || ship.GetLingerSteps() >= system->MinimumFleetPeriod() / 4)
			return false;

		ship.Linger();
		return true;
	}

	// Constants for the invisible fence timer.
	const int FENCE_DECAY = 4;
	const int FENCE_MAX = 600;

	// An offset to prevent the ship from being not quite over the point to departure.
	const double SAFETY_OFFSET = 1.;

	// The minimum speed advantage a ship has to have to consider running away.
	const double SAFETY_MULTIPLIER = 1.1;

	// If a ship's velocity is below this value, the ship is considered stopped.
	constexpr double VELOCITY_ZERO = .001;
}



AI::AI(PlayerInfo &player, const List<Ship> &ships, const List<Minable> &minables, const List<Flotsam> &flotsam)
	: player(player), ships(ships), minables(minables), flotsam(flotsam), routeCache()
{
	// Allocate a starting amount of hardpoints for ships.
	firingCommands.SetHardpoints(12);
	RegisterDerivedConditions(player.Conditions());
}



// Fleet commands from the player.
void AI::IssueFormationChange(PlayerInfo &player)
{
	// Figure out what ships we are giving orders to
	vector<Ship *> targetShips = GetShipsForFormationCommand(player);
	if(targetShips.empty())
		return;

	const auto &formationPatterns = GameData::Formations();
	if(formationPatterns.empty())
	{
		Messages::Add(*GameData::Messages().Get("no formations"));
		return;
	}

	// First check which and how many formation patterns we have in the current set of selected ships.
	// If there is more than 1 pattern in the selection, then this command will change all selected
	// ships to use the pattern that was found first. If there is only 1 pattern, then this command
	// will change all selected ships to use the next pattern available.
	const FormationPattern *toSet = nullptr;
	bool multiplePatternsSet = false;
	for(Ship *ship : targetShips)
	{
		const FormationPattern *shipsPattern = ship->GetFormationPattern();
		if(shipsPattern)
		{
			if(!toSet)
				toSet = shipsPattern;
			else if(toSet != shipsPattern)
			{
				multiplePatternsSet = true;
				break;
			}
		}
	}

	// Now determine what formation pattern to set.
	if(!toSet)
		// If no pattern was set at all, then we set the first one from the set of formationPatterns.
		toSet = &(formationPatterns.begin()->second);
	else if(!multiplePatternsSet)
	{
		// If only one pattern was found, then select the next pattern (or clear the pattern if there is no next).
		auto it = formationPatterns.find(toSet->TrueName());
		if(it != formationPatterns.end())
			++it;
		toSet = (it == formationPatterns.end() ? nullptr : &(it->second));
	}
	// If more than one formation was found on the ships, then we set the pattern it to the first one found.
	// No code is needed here for this option, since all variables are already set to just apply the change below.

	// Now set the pattern on the selected ships, and tell them to gather.
	for(Ship *ship : targetShips)
	{
		ship->SetFormationPattern(toSet);
		orders[ship].Add(OrderSingle{Orders::Types::GATHER});
		orders[ship].SetTargetShip(player.FlagshipPtr());
	}

	unsigned int count = targetShips.size();
	string message = to_string(count) + (count == 1 ? " ship" : " ships") + " will ";
	message += toSet ? ("assume \"" + toSet->TrueName() + "\" formation.") : "no longer fly in formation.";
	Messages::Add({message, GameData::MessageCategories().Get("low")});
}



void AI::IssueShipTarget(const shared_ptr<Ship> &target)
{
	bool isEnemy = target->GetGovernment()->IsEnemy();
	// TODO: There's an error in the code style checker that flags using {} instead of () below.
	OrderSingle newOrder(isEnemy ?
		target->IsDisabled() ? Orders::Types::FINISH_OFF : Orders::Types::ATTACK
		: Orders::Types::KEEP_STATION);
	newOrder.SetTargetShip(target);
	IssueOrder(newOrder,
		(isEnemy ? "focusing fire on" : "following") + (" \"" + target->GivenName() + "\"."));
}



void AI::IssueAsteroidTarget(const shared_ptr<Minable> &targetAsteroid)
{
	OrderSingle newOrder{Orders::Types::MINE};
	newOrder.SetTargetAsteroid(targetAsteroid);
	IssueOrder(newOrder,
		"focusing fire on " + targetAsteroid->DisplayName() + " " + targetAsteroid->Noun() + ".");
}



void AI::IssueMoveTarget(const Point &target, const System *moveToSystem)
{
	OrderSingle newOrder{Orders::Types::MOVE_TO};
	newOrder.SetTargetPoint(target);
	newOrder.SetTargetSystem(moveToSystem);
	string description = "moving to the given location";
	description += player.GetSystem() == moveToSystem ? "." : (" in the " + moveToSystem->DisplayName() + " system.");
	IssueOrder(newOrder, description);
}



// Commands issued via the keyboard (mostly, to the flagship).
void AI::UpdateKeys(PlayerInfo &player, const Command &activeCommands)
{
	const Ship *flagship = player.Flagship();
	if(!flagship || flagship->IsDestroyed())
		return;

	escortsUseAmmo = Preferences::Has("Escorts expend ammo");
	escortsAreFrugal = Preferences::Has("Escorts use ammo frugally");

	if(!autoPilot.Has(Command::STOP) && activeCommands.Has(Command::STOP)
			&& flagship->Velocity().Length() > VELOCITY_ZERO)
		Messages::Add(*GameData::Messages().Get("coming to a stop"));

	autoPilot |= activeCommands;
	if(activeCommands.Has(AutopilotCancelCommands()))
	{
		bool canceled = (autoPilot.Has(Command::JUMP) && !activeCommands.Has(Command::JUMP | Command::FLEET_JUMP));
		canceled |= (autoPilot.Has(Command::STOP) && !activeCommands.Has(Command::STOP));
		canceled |= (autoPilot.Has(Command::LAND) && !activeCommands.Has(Command::LAND));
		canceled |= (autoPilot.Has(Command::BOARD) && !activeCommands.Has(Command::BOARD));
		if(canceled)
			Messages::Add(*GameData::Messages().Get("disengaging autopilot"));
		autoPilot.Clear();
	}

	// Only toggle the "cloak" command if one of your ships has a cloaking device.
	if(activeCommands.Has(Command::CLOAK))
		for(const auto &it : player.Ships())
			if(!it->IsParked() && it->CloakingSpeed())
			{
				isCloaking = !isCloaking;
				Messages::Add(*GameData::Messages().Get(isCloaking ?
					"engaging cloaking device" : "disengaging cloaking device"));
				break;
			}

	// Toggle your secondary weapon.
	if(activeCommands.Has(Command::SELECT))
		player.SelectNextSecondary();

	// The commands below here only apply if you have escorts or fighters.
	if(player.Ships().size() < 2)
		return;

	// Toggle the "deploy" command for the fleet or selected ships.
	if(activeCommands.Has(Command::DEPLOY))
		IssueDeploy(player);

	// The gather command controls formation flying when combined with shift.
	const bool shift = activeCommands.Has(Command::SHIFT);
	if(shift && activeCommands.Has(Command::GATHER))
		IssueFormationChange(player);

	shared_ptr<Ship> target = flagship->GetTargetShip();
	shared_ptr<Minable> targetAsteroid = flagship->GetTargetAsteroid();
	if(activeCommands.Has(Command::FIGHT) && target && !target->IsYours() && !shift)
	{
		OrderSingle newOrder{target->IsDisabled() ? Orders::Types::FINISH_OFF : Orders::Types::ATTACK};
		newOrder.SetTargetShip(target);
		IssueOrder(newOrder, "focusing fire on \"" + target->GivenName() + "\".");
	}
	else if(activeCommands.Has(Command::FIGHT) && !shift && targetAsteroid)
		IssueAsteroidTarget(targetAsteroid);
	if(activeCommands.Has(Command::HOLD_FIRE) && !shift)
	{
		OrderSingle newOrder{Orders::Types::HOLD_FIRE};
		IssueOrder(newOrder, "holding fire.");
	}
	if(activeCommands.Has(Command::HOLD_POSITION) && !shift)
	{
		OrderSingle newOrder{Orders::Types::HOLD_POSITION};
		IssueOrder(newOrder, "holding position.");
	}
	if(activeCommands.Has(Command::GATHER) && !shift)
	{
		OrderSingle newOrder{Orders::Types::GATHER};
		newOrder.SetTargetShip(player.FlagshipPtr());
		IssueOrder(newOrder, "gathering around your flagship.");
	}

	// Get rid of any invalid orders. Carried ships will retain orders in case they are deployed.
	for(auto it = orders.begin(); it != orders.end(); )
	{
		it->second.Validate(it->first, flagship->GetSystem());
		if(it->second.Empty())
		{
			it = orders.erase(it);
			continue;
		}
		++it;
	}
}



void AI::UpdateEvents(const list<ShipEvent> &events)
{
	for(const ShipEvent &event : events)
	{
		const auto &target = event.Target();
		if(!target)
			continue;

		if(event.Actor())
		{
			actions[event.Actor()][target] |= event.Type();
			if(event.TargetGovernment())
				notoriety[event.Actor()][event.TargetGovernment()] |= event.Type();
		}

		const auto &actorGovernment = event.ActorGovernment();
		if(actorGovernment)
		{
			governmentActions[actorGovernment][target] |= event.Type();
			if(actorGovernment->IsPlayer() && event.TargetGovernment())
			{
				int &bitmap = playerActions[target];
				int newActions = event.Type() - (event.Type() & bitmap);
				bitmap |= event.Type();
				// If you provoke the same ship twice, it should have an effect both times.
				if(event.Type() & ShipEvent::PROVOKE)
					newActions |= ShipEvent::PROVOKE;
				event.TargetGovernment()->Offend(newActions, target->CrewValue());
			}
		}
	}
}



// Remove records of what happened in the previous system, now that
// the player has entered a new one.
void AI::Clean()
{
	// Records of what various AI ships and factions have done.
	actions.clear();
	notoriety.clear();
	governmentActions.clear();
	scanPermissions.clear();
	playerActions.clear();
	swarmCount.clear();
	fenceCount.clear();
	cargoScans.clear();
	outfitScans.clear();
	scanTime.clear();
	miningAngle.clear();
	miningRadius.clear();
	miningTime.clear();
	appeasementThreshold.clear();
	boarders.clear();
	routeCache.clear();
	// Records for formations flying around lead ships and other objects.
	formations.clear();
	// Records that affect the combat behavior of various governments.
	shipStrength.clear();
	enemyStrength.clear();
	allyStrength.clear();
}



// Clear ship orders and assistance requests. These should be done
// when the player lands, but not when they change systems.
void AI::ClearOrders()
{
	helperList.clear();
	orders.clear();
}



void AI::Step(Command &activeCommands)
{
	// First, figure out the comparative strengths of the present governments.
	const System *playerSystem = player.GetSystem();
	map<const Government *, int64_t> strength;
	UpdateStrengths(strength, playerSystem);
	CacheShipLists();

	// Update the counts of how long ships have been outside the "invisible fence."
	// If a ship ceases to exist, this also ensures that it will be removed from
	// the fence count map after a few seconds.
	for(auto it = fenceCount.begin(); it != fenceCount.end(); )
	{
		it->second -= FENCE_DECAY;
		if(it->second < 0)
			it = fenceCount.erase(it);
		else
			++it;
	}
	for(const auto &it : ships)
	{
		const System *system = it->GetActualSystem();
		if(system && it->Position().Length() >= system->InvisibleFenceRadius())
		{
			int &value = fenceCount[&*it];
			value = min(FENCE_MAX, value + FENCE_DECAY + 1);
		}
	}

	// Allow all formation-positioners to handle their internal administration to
	// prepare for the next cycle.
	for(auto &bodyIt : formations)
		for(auto &positionerIt : bodyIt.second)
			positionerIt.second.Step();

	const Ship *flagship = player.Flagship();
	step = (step + 1) & 31;
	int targetTurn = 0;
	int minerCount = 0;
	const int maxMinerCount = minables.empty() ? 0 : 9;
	bool opportunisticEscorts = !Preferences::Has("Turrets focus fire");
	bool fightersRetreat = Preferences::Has("Damaged fighters retreat");
	const int npcMaxMiningTime = GameData::GetGamerules().NPCMaxMiningTime();
	for(const auto &it : ships)
	{
		// A destroyed ship can't do anything.
		if(it->IsDestroyed())
			continue;
		// Skip any carried fighters or drones that are somehow in the list.
		if(!it->GetSystem())
			continue;

		if(it.get() == flagship)
		{
			// Player cannot do anything if the flagship is landing.
			if(!flagship->IsLanding())
				MovePlayer(*it, activeCommands);
			continue;
		}

		const Government *gov = it->GetGovernment();
		const Personality &personality = it->GetPersonality();
		double healthRemaining = it->Health();
		bool isPresent = (it->GetSystem() == playerSystem);
		bool isStranded = IsStranded(*it);
		bool thisIsLaunching = (isPresent && HasDeployments(*it));
		if(isStranded || it->IsDisabled())
		{
			// Attempt to find a friendly ship to render assistance.
			if(!it->GetPersonality().IsDerelict())
				AskForHelp(*it, isStranded, flagship);

			if(it->IsDisabled())
			{
				// Ships other than escorts should deploy fighters if disabled.
				if(!it->IsYours() || thisIsLaunching)
				{
					it->SetCommands(Command::DEPLOY);
					Deploy(*it, !(it->IsYours() && fightersRetreat));
				}
				// Avoid jettisoning cargo as soon as this ship is repaired.
				if(personality.IsAppeasing())
				{
					double health = .5 * it->Shields() + it->Hull();
					double &threshold = appeasementThreshold[it.get()];
					threshold = max((1. - health) + .1, threshold);
				}
				continue;
			}
		}
		// Overheated ships are effectively disabled, and cannot fire, cloak, etc.
		if(it->IsOverheated())
			continue;

		Command command;
		firingCommands.SetHardpoints(it->Weapons().size());
		if(it->IsYours())
		{
			if(it->HasBays() && thisIsLaunching)
			{
				// If this is a carrier, launch whichever of its fighters are at
				// good enough health to survive a fight.
				command |= Command::DEPLOY;
				Deploy(*it, !fightersRetreat);
			}
			if(isCloaking)
				command |= Command::CLOAK;
		}

		// Cloak if the AI considers it appropriate.
		if(!it->IsYours() || !isCloaking)
			if(DoCloak(*it, command))
			{
				// The ship chose to retreat from its target, e.g. to repair.
				it->SetCommands(command);
				continue;
			}

		shared_ptr<Ship> parent = it->GetParent();
		if(parent && parent->IsDestroyed())
		{
			// An NPC that loses its fleet leader should attempt to
			// follow that leader's parent. For most mission NPCs,
			// this is the player. Any regular NPCs and mission NPCs
			// with "personality uninterested" become independent.
			parent = parent->GetParent();
			it->SetParent(parent);
		}

		// Pick a target and automatically fire weapons.
		shared_ptr<Ship> target = it->GetTargetShip();
		shared_ptr<Minable> targetAsteroid = it->GetTargetAsteroid();
		shared_ptr<Flotsam> targetFlotsam = it->GetTargetFlotsam();
		if(isPresent && it->IsYours() && targetFlotsam && FollowOrders(*it, command))
			continue;
		// Determine if this ship was trying to scan its previous target. If so, keep
		// track of this ship's scan time and count.
		if(isPresent && !it->IsYours())
		{
			// Assume that if the target is friendly, not disabled, of a different
			// government to this ship, and this ship has scanning capabilities
			// then it was attempting to scan the target. This isn't a perfect
			// assumption, but should be good enough for now.
			bool cargoScan = it->Attributes().Get("cargo scan power");
			bool outfitScan = it->Attributes().Get("outfit scan power");
			if((cargoScan || outfitScan) && target && !target->IsDisabled()
				&& !target->GetGovernment()->IsEnemy(gov) && target->GetGovernment() != gov)
			{
				++scanTime[&*it];
				if(it->CargoScanFraction() == 1.)
					cargoScans[&*it].insert(&*target);
				if(it->OutfitScanFraction() == 1.)
					outfitScans[&*it].insert(&*target);
			}
		}
		if(isPresent && !personality.IsSwarming())
		{
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			if(targetTurn == step || !target || target->IsDestroyed() || (target->IsDisabled() &&
					(personality.Disables() || (!FighterHitHelper::IsValidTarget(target.get()) && !personality.IsVindictive())))
					|| (target->IsFleeing() && personality.IsMerciful()) || !target->IsTargetable())
			{
				target = FindTarget(*it);
				it->SetTargetShip(target);
			}
		}
		if(isPresent)
		{
			AimTurrets(*it, firingCommands, it->IsYours() ? opportunisticEscorts : personality.IsOpportunistic());
			if(targetAsteroid)
				AutoFire(*it, firingCommands, *targetAsteroid);
			else
				AutoFire(*it, firingCommands);
		}

		// If this ship is hyperspacing, or in the act of
		// launching or landing, it can't do anything else.
		if(it->IsHyperspacing() || it->Zoom() < 1.)
		{
			it->SetCommands(command);
			it->SetCommands(firingCommands);
			continue;
		}

		// Run away if your hostile target is not disabled
		// and you are either badly damaged or have run out of ammo.
		// Player ships never stop targeting hostiles, while hostile mission NPCs will
		// do so only if they are allowed to leave.
		const bool shouldFlee = ShouldFlee(*it);
		if(!it->IsYours() && shouldFlee && target && target->GetGovernment()->IsEnemy(gov) && !target->IsDisabled()
			&& (!it->GetParent() || !it->GetParent()->GetGovernment()->IsEnemy(gov)))
		{
			// Make sure the ship has somewhere to flee to.
			const System *system = it->GetSystem();
			if(it->JumpsRemaining() && (!system->Links().empty() || it->JumpNavigation().HasJumpDrive()))
				target.reset();
			else
				for(const StellarObject &object : system->Objects())
					if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsInhabited()
							&& object.GetPlanet()->CanLand(*it))
					{
						target.reset();
						break;
					}

			if(target)
				// This ship has nowhere to flee to: Stop fleeing.
				it->SetFleeing(false);
			else
			{
				// This ship has somewhere to flee to: Remove target and mark this ship as fleeing.
				it->SetTargetShip(target);
				it->SetFleeing();
			}
		}
		else if(it->IsFleeing())
			it->SetFleeing(false);

		// Special actions when a ship is heavily damaged:
		if(healthRemaining < RETREAT_HEALTH + .25)
		{
			// Cowards abandon their fleets.
			if(parent && personality.IsCoward())
			{
				parent.reset();
				it->SetParent(parent);
			}
			// Appeasing ships jettison cargo to distract their pursuers.
			if(personality.IsAppeasing() && it->Cargo().Used())
				DoAppeasing(it, &appeasementThreshold[it.get()]);
		}

		// If recruited to assist a ship, follow through on the commitment
		// instead of ignoring it due to other personality traits.
		shared_ptr<Ship> shipToAssist = it->GetShipToAssist();
		if(shipToAssist)
		{
			if(shipToAssist->IsDestroyed() || shipToAssist->GetSystem() != it->GetSystem()
					|| shipToAssist->IsLanding() || shipToAssist->IsHyperspacing()
					|| shipToAssist->GetGovernment()->IsEnemy(gov)
					|| (!shipToAssist->IsDisabled() && !shipToAssist->NeedsFuel() && !shipToAssist->NeedsEnergy()))
			{
				if(target == shipToAssist)
				{
					target.reset();
					it->SetTargetShip(nullptr);
				}
				shipToAssist.reset();
				it->SetShipToAssist(nullptr);
			}
			else if(!it->IsBoarding())
			{
				MoveTo(*it, command, shipToAssist->Position(), shipToAssist->Velocity(), 40., .8);
				command |= Command::BOARD;
			}

			if(shipToAssist)
			{
				it->SetTargetShip(shipToAssist);
				it->SetCommands(command);
				it->SetCommands(firingCommands);
				continue;
			}
		}

		// This ship may have updated its target ship.
		double targetDistance = numeric_limits<double>::infinity();
		target = it->GetTargetShip();
		if(target)
			targetDistance = target->Position().Distance(it->Position());

		// Special case: if the player's flagship tries to board a ship to
		// refuel it, that escort should hold position for boarding.
		isStranded |= (flagship && it == flagship->GetTargetShip() && CanBoard(*flagship, *it)
			&& autoPilot.Has(Command::BOARD));

		// Stranded ships that have a helper need to stop and be assisted.
		bool strandedWithHelper = isStranded &&
			(HasHelper(*it, NeedsFuel(*it), NeedsEnergy(*it)) || it->GetPersonality().IsDerelict() || it->IsYours());

		// Behave in accordance with personality traits.
		if(isPresent && personality.IsSwarming() && !strandedWithHelper)
		{
			// Swarming ships should not wait for (or be waited for by) any ship.
			if(parent)
			{
				parent.reset();
				it->SetParent(parent);
			}
			// Flock between allied, in-system ships.
			DoSwarming(*it, command, target);
			it->SetCommands(command);
			it->SetCommands(firingCommands);
			continue;
		}

		if(isPresent && personality.IsSecretive())
		{
			if(DoSecretive(*it, command))
			{
				it->SetCommands(command);
				continue;
			}
		}

		// Surveillance NPCs with enforcement authority (or those from
		// missions) should perform scans and surveys of the system.
		if(isPresent && personality.IsSurveillance() && !strandedWithHelper
				&& (scanPermissions[gov] || it->IsSpecial()))
		{
			DoSurveillance(*it, command, target);
			it->SetCommands(command);
			it->SetCommands(firingCommands);
			continue;
		}

		// Ships that harvest flotsam prioritize it over stopping to be refueled.
		if(isPresent && personality.Harvests() && DoHarvesting(*it, command))
		{
			it->SetCommands(command);
			it->SetCommands(firingCommands);
			continue;
		}

		// Attacking a hostile ship, fleeing and stopping to be refueled are more important than mining.
		if(isPresent && personality.IsMining() && !shouldFlee && !target && !strandedWithHelper && maxMinerCount)
		{
			// Miners with free cargo space and available mining time should mine. Mission NPCs
			// should mine even if there are other miners or they have been mining a while.
			if(it->Cargo().Free() >= 5 && IsArmed(*it) && (it->IsSpecial()
					|| (++miningTime[&*it] < npcMaxMiningTime && ++minerCount < maxMinerCount)))
			{
				if(it->HasBays())
				{
					command |= Command::DEPLOY;
					Deploy(*it, false);
				}
				DoMining(*it, command);
				it->SetCommands(command);
				it->SetCommands(firingCommands);
				continue;
			}
			// Fighters and drones should assist their parent's mining operation if they cannot
			// carry ore, and the asteroid is near enough that the parent can harvest the ore.
			if(it->CanBeCarried() && parent && miningTime[parent.get()] < 3601)
			{
				const shared_ptr<Minable> &minable = parent->GetTargetAsteroid();
				if(minable && minable->Position().Distance(parent->Position()) < 600.)
				{
					it->SetTargetAsteroid(minable);
					MoveToAttack(*it, command, *minable);
					AutoFire(*it, firingCommands, *minable);
					it->SetCommands(command);
					it->SetCommands(firingCommands);
					continue;
				}
			}
			it->SetTargetAsteroid(nullptr);
		}

		// Handle carried ships:
		if(it->CanBeCarried())
		{
			// A carried ship must belong to the same government as its parent to dock with it.
			bool hasParent = parent && !parent->IsDestroyed() && parent->GetGovernment() == gov;
			bool inParentSystem = hasParent && parent->GetSystem() == it->GetSystem();
			// NPCs may take 30 seconds or longer to find a new parent. Player
			// owned fighter shouldn't take more than a few seconds.
			bool findNewParent = it->IsYours() ? !Random::Int(30) : !Random::Int(1800);
			bool parentHasSpace = inParentSystem && parent->BaysFree(it->Attributes().Category());
			if(findNewParent && parentHasSpace && it->IsYours())
				parentHasSpace = parent->CanCarry(*it);
			if(!hasParent || (!inParentSystem && !it->JumpNavigation().JumpFuel()) || (!parentHasSpace && findNewParent))
			{
				// Find the possible parents for orphaned fighters and drones.
				auto parentChoices = vector<shared_ptr<Ship>>{};
				parentChoices.reserve(ships.size() * .1);
				auto getParentFrom = [&it, &gov, &parentChoices](const list<shared_ptr<Ship>> &otherShips) -> shared_ptr<Ship>
				{
					for(const auto &other : otherShips)
						if(other->GetGovernment() == gov && other->GetSystem() == it->GetSystem() && !other->CanBeCarried())
						{
							if(!other->IsDisabled() && other->CanCarry(*it))
								return other;
							else
								parentChoices.emplace_back(other);
						}
					return shared_ptr<Ship>();
				};
				// Mission ships should only pick amongst ships from the same mission.
				auto missionIt = it->IsSpecial() && !it->IsYours()
					? find_if(player.Missions().begin(), player.Missions().end(),
						[&it](const Mission &m) { return m.HasShip(it); })
					: player.Missions().end();

				shared_ptr<Ship> newParent;
				if(missionIt != player.Missions().end())
				{
					auto &npcs = missionIt->NPCs();
					for(const auto &npc : npcs)
					{
						// Don't reparent to NPC ships that have not been spawned.
						if(!npc.ShouldSpawn())
							continue;
						newParent = getParentFrom(npc.Ships());
						if(newParent)
							break;
					}
				}
				else
					newParent = getParentFrom(ships);

				// If a new parent was found, then this carried ship should always reparent
				// as a ship of its own government is in-system and has space to carry it.
				if(newParent)
					parent = newParent;
				// Otherwise, if one or more in-system ships of the same government were found,
				// this carried ship should flock with one of them, even if they can't carry it.
				else if(!parentChoices.empty())
					parent = parentChoices[Random::Int(parentChoices.size())];
				// Player-owned carriables that can't be carried and have no ships to flock with
				// should keep their current parent, or if it is destroyed, their parent's parent.
				else if(it->IsYours())
				{
					if(parent && parent->IsDestroyed())
						parent = parent->GetParent();
				}
				// All remaining non-player ships should forget their previous parent entirely.
				else
					parent.reset();

				// Player-owned carriables should defer to player carrier if
				// selected parent can't carry it. This is necessary to prevent
				// fighters from jumping around fleet when there's not enough
				// bays.
				if(it->IsYours() && parent && parent->GetParent() && !parent->CanCarry(*it))
					parent = parent->GetParent();

				if(it->GetParent() != parent)
					it->SetParent(parent);
			}
			// Otherwise, check if this ship wants to return to its parent (e.g. to repair).
			else if(parentHasSpace && ShouldDock(*it, *parent, playerSystem))
			{
				it->SetTargetShip(parent);
				MoveTo(*it, command, parent->Position(), parent->Velocity(), 40., .8);
				command |= Command::BOARD;
				it->SetCommands(command);
				it->SetCommands(firingCommands);
				continue;
			}
			// If we get here, it means that the ship has not decided to return
			// to its mothership. So, it should continue to be deployed.
			command |= Command::DEPLOY;
		}
		// If this ship has decided to recall all of its fighters because combat has ceased,
		// it comes to a stop to facilitate their reboarding process.
		bool mustRecall = false;
		if(!target && it->HasBays() && !(it->IsYours() ?
				thisIsLaunching : it->Commands().Has(Command::DEPLOY)))
			for(const weak_ptr<Ship> &ptr : it->GetEscorts())
			{
				shared_ptr<const Ship> escort = ptr.lock();
				// Note: HasDeployOrder is always `false` for NPC ships, as it is solely used for player ships.
				if(escort && escort->CanBeCarried() && !escort->HasDeployOrder() && escort->GetSystem() == it->GetSystem()
						&& !escort->IsDisabled() && it->BaysFree(escort->Attributes().Category()))
				{
					mustRecall = true;
					break;
				}
			}

		// Construct movement / navigation commands as appropriate for the ship.
		if(mustRecall || (strandedWithHelper && !it->GetPersonality().IsDerelict()))
		{
			// Stopping to let fighters board or to be refueled takes priority
			// even over following orders from the player.
			if(it->Velocity().Length() > VELOCITY_ZERO || !target)
				Stop(*it, command);
			else
			{
				command.SetTurn(TurnToward(*it, TargetAim(*it)));
				it->SetVelocity({0., 0.});
			}
		}
		else if(FollowOrders(*it, command))
		{
			// If this is an escort and it followed orders, its only final task
			// is to convert completed MOVE_TO orders into HOLD_POSITION orders.
			UpdateOrders(*it);
		}
		// Hostile "escorts" (i.e. NPCs that are trailing you) only revert to
		// escort behavior when in a different system from you. Otherwise,
		// the behavior depends on what the parent is doing, whether there
		// are hostile targets nearby, and whether the escort has any
		// immediate needs (like refueling).
		else if(!parent)
			MoveIndependent(*it, command);
		else if(parent->GetSystem() != it->GetSystem())
		{
			if(personality.IsStaying() || !it->Attributes().Get("fuel capacity"))
				MoveIndependent(*it, command);
			else
				MoveEscort(*it, command);
		}
		// From here down, we're only dealing with ships that have a "parent"
		// which is in the same system as them.
		else if(parent->GetGovernment()->IsEnemy(gov))
		{
			// Fight your target, if you have one.
			if(target)
				MoveIndependent(*it, command);
			// Otherwise try to find and fight your parent. If your parent
			// can't be both targeted and pursued, then don't follow them.
			else if(parent->IsTargetable() && CanPursue(*it, *parent))
				MoveEscort(*it, command);
			else
				MoveIndependent(*it, command);
		}
		else if(parent->IsDisabled() && !it->CanBeCarried())
		{
			// Your parent is disabled, and is in this system. If you have enemy
			// targets present, fight them. Otherwise, repair your parent.
			if(target)
				MoveIndependent(*it, command);
			else if(!parent->GetPersonality().IsDerelict())
				it->SetShipToAssist(parent);
			else
				CircleAround(*it, command, *parent);
		}
		else if(personality.IsStaying())
			MoveIndependent(*it, command);
		// This is a friendly escort. If the parent is getting ready to
		// jump, always follow.
		else if(parent->Commands().Has(Command::JUMP) && !strandedWithHelper)
			MoveEscort(*it, command);
		// Timid ships always stay near their parent. Injured player
		// escorts will stay nearby until they have repaired a bit.
		else if((personality.IsTimid() || (it->IsYours() && healthRemaining < RETREAT_HEALTH))
				&& parent->Position().Distance(it->Position()) > 500.)
			MoveEscort(*it, command);
		// Otherwise, attack targets depending on your hunting attribute.
		else if(target && (targetDistance < 2000. || personality.IsHunting()))
			MoveIndependent(*it, command);
		// This ship does not feel like fighting.
		else
			MoveEscort(*it, command);

		// Force ships that are overlapping each other to "scatter":
		DoScatter(*it, command);

		it->SetCommands(command);
		it->SetCommands(firingCommands);
	}
}



void AI::SetMousePosition(Point position)
{
	mousePosition = position;
}



// Get the in-system strength of each government's allies and enemies.
int64_t AI::AllyStrength(const Government *government) const
{
	auto it = allyStrength.find(government);
	return (it == allyStrength.end() ? 0 : it->second);
}



int64_t AI::EnemyStrength(const Government *government) const
{
	auto it = enemyStrength.find(government);
	return (it == enemyStrength.end() ? 0 : it->second);
}



// Find nearest landing location.
const StellarObject *AI::FindLandingLocation(const Ship &ship, const bool refuel)
{
	const StellarObject *target = nullptr;
	const System *system = ship.GetSystem();
	if(system)
	{
		// Determine which, if any, planet with fuel or without fuel is closest.
		double closest = numeric_limits<double>::infinity();
		const Point &p = ship.Position();
		for(const StellarObject &object : system->Objects())
		{
			const Planet *planet = object.GetPlanet();
			if(object.HasSprite() && object.HasValidPlanet() && !planet->IsWormhole()
					&& planet->CanLand(ship) && planet->HasFuelFor(ship) == refuel)
			{
				double distance = p.Distance(object.Position());
				if(distance < closest)
				{
					target = &object;
					closest = distance;
				}
			}
		}
	}
	return target;
}



AI::RouteCacheKey::RouteCacheKey(
	const System *from, const System *to, const Government *gov, double jumpDistance,
	JumpType jumpType, const vector<string> &wormholeKeys)
		: from(from), to(to), gov(gov), jumpDistance(jumpDistance), jumpType(jumpType), wormholeKeys(wormholeKeys)
{
}



size_t AI::RouteCacheKey::HashFunction::operator()(RouteCacheKey const &key) const
{
	// Used by unordered_map to determine equivalence.
	int shift = 0;
	size_t hash = std::hash<string>()(key.from->TrueName());
	hash ^= std::hash<string>()(key.to->TrueName()) << ++shift;
	hash ^= std::hash<string>()(key.gov->TrueName()) << ++shift;
	hash ^= std::hash<int>()(key.jumpDistance) << ++shift;
	hash ^= std::hash<int>()(static_cast<std::size_t>(key.jumpType)) << ++shift;
	for(const string &k : key.wormholeKeys)
		hash ^= std::hash<string>()(k) << ++shift;;
	return hash;
}



bool AI::RouteCacheKey::operator==(const RouteCacheKey &other) const
{
	// Used by unordered_map to determine equivalence.
	return from == other.from
		&& to == other.to
		&& gov == other.gov
		&& jumpDistance == other.jumpDistance
		&& jumpType == other.jumpType
		&& wormholeKeys == other.wormholeKeys;
}



bool AI::RouteCacheKey::operator!=(const RouteCacheKey &other) const
{
	// Used by unordered_map to determine equivalence.
	return !(*this == other);
}



// Check if the given target can be pursued by this ship.
bool AI::CanPursue(const Ship &ship, const Ship &target) const
{
	// If this ship does not care about the "invisible fence", it can always pursue.
	if(ship.GetPersonality().IsUnconstrained())
		return true;

	// Owned ships ignore fence.
	if(ship.IsYours())
		return true;

	// Check if the target is beyond the "invisible fence" for this system.
	const auto fit = fenceCount.find(&target);
	if(fit == fenceCount.end())
		return true;
	else
		return (fit->second != FENCE_MAX);
}



// Check if the ship is being helped, and if not, ask for help.
void AI::AskForHelp(Ship &ship, bool &isStranded, const Ship *flagship)
{
	bool needsFuel = NeedsFuel(ship);
	bool needsEnergy = NeedsEnergy(ship);
	if(HasHelper(ship, needsFuel, needsEnergy))
		isStranded = true;
	else if(!Random::Int(30))
	{
		const Government *gov = ship.GetGovernment();
		bool hasEnemy = false;

		vector<Ship *> canHelp;
		canHelp.reserve(ships.size());
		for(const auto &helper : ships)
		{
			// Never ask yourself for help.
			if(helper.get() == &ship)
				continue;

			// If any able enemies of this ship are in its system, it cannot call for help.
			const System *system = ship.GetSystem();
			if(helper->GetGovernment()->IsEnemy(gov) && flagship && system == flagship->GetSystem())
			{
				// Disabled, overheated, or otherwise untargetable ships pose no threat.
				bool harmless = helper->IsDisabled() || (helper->IsOverheated() && helper->Heat() >= 1.1)
						|| !helper->IsTargetable();
				hasEnemy |= (system == helper->GetSystem() && !harmless);
				if(hasEnemy)
					break;
			}

			// Check if this ship is logically able to help.
			// If the ship is already assisting someone else, it cannot help this ship.
			if(helper->GetShipToAssist() && helper->GetShipToAssist().get() != &ship)
				continue;
			// If the ship is mining or chasing flotsam, it cannot help this ship.
			if(helper->GetTargetAsteroid() || helper->GetTargetFlotsam())
				continue;
			// Your escorts only help other escorts, and your flagship never helps.
			if((helper->IsYours() && !ship.IsYours()) || helper.get() == flagship)
				continue;
			// Your escorts should not help each other if already under orders.
			auto foundOrders = orders.find(helper.get());
			if(foundOrders != orders.end())
			{
				const OrderSet &helperOrders = foundOrders->second;
				// If your own escorts become disabled, then your mining fleet
				// should prioritize repairing escorts instead of mining or
				// harvesting flotsam.
				if(helper->IsYours() && ship.IsYours()
						&& !(helperOrders.Has(Orders::Types::MINE) || helperOrders.Has(Orders::Types::HARVEST)))
					continue;
			}

			// Check if this ship is physically able to help.
			if(!CanHelp(ship, *helper, needsFuel, needsEnergy))
				continue;

			// Prefer fast ships over slow ones.
			canHelp.insert(canHelp.end(), 1 + .3 * helper->MaxVelocity(), helper.get());
		}

		if(!hasEnemy && !canHelp.empty())
		{
			Ship *helper = canHelp[Random::Int(canHelp.size())];
			helper->SetShipToAssist((&ship)->shared_from_this());
			helperList[&ship] = helper->shared_from_this();
			isStranded = true;
		}
		else
			isStranded = false;
	}
	else
		isStranded = false;
}



// Determine if the selected ship is physically able to render assistance.
bool AI::CanHelp(const Ship &ship, const Ship &helper, const bool needsFuel, const bool needsEnergy) const
{
	// A ship being assisted cannot assist.
	if(helperList.find(&helper) != helperList.end())
		return false;

	// Fighters, drones, and disabled / absent ships can't offer assistance.
	if(helper.CanBeCarried() || helper.GetSystem() != ship.GetSystem() ||
			(helper.GetGovernment() != ship.GetGovernment() && helper.CannotAct(Ship::ActionType::COMMUNICATION))
			|| helper.IsOverheated() || helper.IsHyperspacing())
		return false;

	// An enemy cannot provide assistance, and only ships of the same government will repair disabled ships.
	if(helper.GetGovernment()->IsEnemy(ship.GetGovernment())
			|| (ship.IsDisabled() && helper.GetGovernment() != ship.GetGovernment()))
		return false;

	// If the helper has insufficient fuel or energy, it cannot help this ship unless this ship is also disabled.
	if(!ship.IsDisabled() && ((needsFuel && !helper.CanRefuel(ship)) || (needsEnergy && !helper.CanGiveEnergy(ship))))
		return false;

	// For player's escorts, check if the player knows the helper's language.
	if(ship.IsYours() && !helper.GetGovernment()->Language().empty()
			&& !player.Conditions().Get("language: " + helper.GetGovernment()->Language()))
		return false;

	return true;
}



bool AI::HasHelper(const Ship &ship, const bool needsFuel, const bool needsEnergy)
{
	// Do we have an existing ship that was asked to assist?
	if(helperList.find(&ship) != helperList.end())
	{
		shared_ptr<Ship> helper = helperList[&ship].lock();
		if(helper && helper->GetShipToAssist().get() == &ship && CanHelp(ship, *helper, needsFuel, needsEnergy))
			return true;
		else
			helperList.erase(&ship);
	}

	return false;
}



// Pick a new target for the given ship.
shared_ptr<Ship> AI::FindTarget(const Ship &ship) const
{
	// If this ship has no government, it has no enemies.
	shared_ptr<Ship> target;
	const Government *gov = ship.GetGovernment();
	if(!gov || ship.GetPersonality().IsPacifist())
		return FindNonHostileTarget(ship);

	bool isYours = ship.IsYours();
	if(isYours)
	{
		auto it = orders.find(&ship);
		if(it != orders.end())
		{
			if(it->second.Has(Orders::Types::ATTACK) || it->second.Has(Orders::Types::FINISH_OFF))
				return it->second.GetTargetShip();
			if(it->second.Has(Orders::Types::HOLD_FIRE))
				return target;
		}
	}

	// If this ship is not armed, do not make it fight.
	double minRange = numeric_limits<double>::infinity();
	double maxRange = 0.;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(weapon && !hardpoint.IsSpecial())
		{
			double range = weapon->Range();
			minRange = min(minRange, range);
			maxRange = max(maxRange, range);
		}
	}
	if(!maxRange)
		return FindNonHostileTarget(ship);

	const Personality &person = ship.GetPersonality();
	shared_ptr<Ship> oldTarget = ship.GetTargetShip();
	if(oldTarget && !oldTarget->IsTargetable())
		oldTarget.reset();
	if(oldTarget && person.IsTimid() && oldTarget->IsDisabled()
			&& ship.Position().Distance(oldTarget->Position()) > 1000.)
		oldTarget.reset();
	// Ships with 'plunders' personality always destroy the ships they have boarded
	// unless they also have either or both of the 'disables' or 'merciful' personalities.
	if(oldTarget && person.Plunders() && !person.Disables() && !person.IsMerciful()
			&& oldTarget->IsDisabled() && Has(ship, oldTarget, ShipEvent::BOARD))
		return oldTarget;
	shared_ptr<Ship> parentTarget;
	if(ship.GetParent() && !ship.GetParent()->GetGovernment()->IsEnemy(gov))
		parentTarget = ship.GetParent()->GetTargetShip();
	if(parentTarget && !parentTarget->IsTargetable())
		parentTarget.reset();

	// Find the closest enemy ship (if there is one). If this ship is "hunting,"
	// it will attack any ship in system. Otherwise, if all its weapons have a
	// range higher than 2000, it will engage ships up to 50% beyond its range.
	// If a ship has short range weapons and is not hunting, it will engage any
	// ship that is within 3000 of it.
	double closest = person.IsHunting() ? numeric_limits<double>::infinity() :
		(minRange > 1000.) ? maxRange * 1.5 : 4000.;
	bool hasNemesis = false;
	bool canPlunder = person.Plunders() && ship.Cargo().Free() && !ship.CanBeCarried();
	// Figure out how strong this ship is.
	int64_t maxStrength = 0;
	auto strengthIt = shipStrength.find(&ship);
	if(!person.IsDaring() && strengthIt != shipStrength.end())
		maxStrength = 2 * strengthIt->second;

	// Get a list of all targetable, hostile ships in this system.
	const auto enemies = GetShipsList(ship, true);
	for(const auto &foe : enemies)
	{
		// If this is a "nemesis" ship and it has found one of the player's
		// ships to target, it will only consider the player's owned fleet,
		// or NPCs being escorted by the player.
		const bool isPotentialNemesis = person.IsNemesis()
				&& (foe->IsYours() || foe->GetPersonality().IsEscort());
		if(hasNemesis && !isPotentialNemesis)
			continue;
		if(!CanPursue(ship, *foe))
			continue;

		// Estimate the range a second from now, so ships prefer foes they are approaching.
		double range = (foe->Position() + 60. * foe->Velocity()).Distance(
			ship.Position() + 60. * ship.Velocity());
		// Prefer the previous target, or the parent's target, if they are nearby.
		if(foe == oldTarget.get() || foe == parentTarget.get())
			range -= 500.;

		// Unless this ship is "daring", it should not chase much stronger ships.
		if(maxStrength && range > 1000. && !foe->IsDisabled())
		{
			const auto otherStrengthIt = shipStrength.find(foe);
			if(otherStrengthIt != shipStrength.end() && otherStrengthIt->second > maxStrength)
				continue;
		}

		// Merciful ships do not attack any ships that are trying to escape.
		if(person.IsMerciful() && foe->IsFleeing())
			continue;

		// Ships which only disable never target already-disabled ships.
		if((person.Disables() || (!person.IsNemesis() && foe != oldTarget.get()))
				&& foe->IsDisabled() && (!canPlunder || Has(ship, foe->shared_from_this(), ShipEvent::BOARD)))
			continue;

		// Ships that don't (or can't) plunder strongly prefer active targets.
		if(!canPlunder)
			range += 5000. * foe->IsDisabled();
		// While those that do, do so only if no "live" enemies are nearby.
		else
		{
			if(any_of(boarders.begin(), boarders.end(), [&ship, &foe](auto &it)
					{ return it.first != &ship && it.second == foe; }))
				continue;
			range += 2000. * (2 * foe->IsDisabled() - !Has(ship, foe->shared_from_this(), ShipEvent::BOARD));
		}

		// Prefer to go after armed targets, especially if you're not a pirate.
		range += 1000. * (!IsArmed(*foe) * (1 + !person.Plunders()));
		// Targets which have plundered this ship's faction earn extra scorn.
		range -= 1000 * Has(*foe, gov, ShipEvent::BOARD);
		// Focus on nearly dead ships.
		range += 500. * (foe->Shields() + foe->Hull());
		// If a target is extremely overheated, focus on ships that can attack back.
		if(foe->IsOverheated())
			range += 3000. * (foe->Heat() - .9);
		if((isPotentialNemesis && !hasNemesis) || range < closest)
		{
			closest = range;
			target = foe->shared_from_this();
			hasNemesis = isPotentialNemesis;
		}
	}

	// With no hostile targets, NPCs with enforcement authority (and any
	// mission NPCs) should consider friendly targets for surveillance.
	if(!isYours && !target && (ship.IsSpecial() || scanPermissions.at(gov)))
		target = FindNonHostileTarget(ship);

	// Vindictive personalities without in-range hostile targets keep firing at an old
	// target (instead of perhaps moving about and finding one that is still alive).
	if(!target && person.IsVindictive())
	{
		target = ship.GetTargetShip();
		if(target && (target->IsCloaked() || target->GetSystem() != ship.GetSystem()))
			target.reset();
	}

	return target;
}



shared_ptr<Ship> AI::FindNonHostileTarget(const Ship &ship) const
{
	shared_ptr<Ship> target;

	// A ship may only make up to 6 successful scans (3 ships scanned if the ship
	// is using both scanners) and spend up to 2.5 minutes searching for scan targets.
	// After that, stop scanning targets. This is so that scanning ships in high spawn
	// rate systems don't build up over time, as they always have a new ship they
	// can try to scan.
	// Ships with the surveillance personality may make up to 12 successful scans and
	// spend 5 minutes searching.
	bool isSurveillance = ship.GetPersonality().IsSurveillance();
	int maxScanCount = isSurveillance ? 12 : 6;
	int searchTime = isSurveillance ? 9000 : 18000;
	// Ships will stop finding new targets to scan after the above scan time, but
	// may continue to pursue a target that they already started scanning for an
	// additional minute.
	int forfeitTime = searchTime + 3600;

	double cargoScan = ship.Attributes().Get("cargo scan power");
	double outfitScan = ship.Attributes().Get("outfit scan power");
	auto cargoScansIt = cargoScans.find(&ship);
	auto outfitScansIt = outfitScans.find(&ship);
	auto scanTimeIt = scanTime.find(&ship);
	int shipScanCount = cargoScansIt != cargoScans.end() ? cargoScansIt->second.size() : 0;
	shipScanCount += outfitScansIt != outfitScans.end() ? outfitScansIt->second.size() : 0;
	int shipScanTime = scanTimeIt != scanTime.end() ? scanTimeIt->second : 0;
	if((cargoScan || outfitScan) && shipScanCount < maxScanCount && shipScanTime < forfeitTime)
	{
		// If this ship already has a target, and is in the process of scanning it, prioritise that,
		// even if the scan time for this ship has exceeded 2.5 minutes.
		shared_ptr<Ship> oldTarget = ship.GetTargetShip();
		if(oldTarget && !oldTarget->IsTargetable())
			oldTarget.reset();
		if(oldTarget)
		{
			// If this ship started scanning this target then continue to scan it
			// unless it has traveled too far outside of the scan range. Surveillance
			// ships will continue to pursue targets regardless of range.
			bool cargoScanInProgress = ship.CargoScanFraction() > 0. && ship.CargoScanFraction() < 1.;
			bool outfitScanInProgress = ship.OutfitScanFraction() > 0. && ship.OutfitScanFraction() < 1.;
			// Divide the distance by 10,000 to normalize to the scan range that
			// scan power provides.
			double range = isSurveillance ? 0. : oldTarget->Position().DistanceSquared(ship.Position()) * .0001;
			if((cargoScanInProgress && range < 2. * cargoScan)
				|| (outfitScanInProgress && range < 2. * outfitScan))
				target = std::move(oldTarget);
		}
		else if(shipScanTime < searchTime)
		{
			// Don't try chasing targets that are too far away from this ship's scan range.
			// Surveillance ships will still prioritize nearby targets here, but if there
			// is no scan target nearby then they will pick a random target in the system
			// to pursue in DoSurveillance.
			double closest = max(cargoScan, outfitScan) * 2.;
			const Government *gov = ship.GetGovernment();
			for(const auto &it : GetShipsList(ship, false))
				if(it->GetGovernment() != gov)
				{
					auto ptr = it->shared_from_this();
					// Scan friendly ships that are as-yet unscanned by this ship's government.
					if((!cargoScan || Has(gov, ptr, ShipEvent::SCAN_CARGO))
							&& (!outfitScan || Has(gov, ptr, ShipEvent::SCAN_OUTFITS)))
						continue;

					// Divide the distance by 10,000 to normalize to the scan range that
					// scan power provides.
					double range = it->Position().DistanceSquared(ship.Position()) * .0001;
					if(range < closest)
					{
						closest = range;
						target = std::move(ptr);
					}
				}
		}
	}

	return target;
}



// Return a list of all targetable ships in the same system as the player that
// match the desired hostility (i.e. enemy or non-enemy). Does not consider the
// ship's current target, as its inclusion may or may not be desired.
vector<Ship *> AI::GetShipsList(const Ship &ship, bool targetEnemies, double maxRange) const
{
	if(maxRange < 0.)
		maxRange = numeric_limits<double>::infinity();

	auto targets = vector<Ship *>();

	// The cached lists are built each step based on the current ships in the player's system.
	const auto &rosters = targetEnemies ? enemyLists : allyLists;

	const auto it = rosters.find(ship.GetGovernment());
	if(it != rosters.end() && !it->second.empty())
	{
		targets.reserve(it->second.size());

		const System *here = ship.GetSystem();
		const Point &p = ship.Position();
		for(const auto &target : it->second)
			if(target->IsTargetable() && target->GetSystem() == here
					&& !(target->IsHyperspacing() && target->Velocity().Length() > 10.)
					&& p.Distance(target->Position()) < maxRange
					&& (ship.IsYours() || !target->GetPersonality().IsMarked())
					&& (target->IsYours() || !ship.GetPersonality().IsMarked()))
				targets.emplace_back(target);
	}

	return targets;
}



// TODO: This should be const when ships are not added and removed from formations in MoveInFormation
bool AI::FollowOrders(Ship &ship, Command &command)
{
	auto it = orders.find(&ship);
	if(it == orders.end())
		return false;

	const OrderSet &shipOrders = it->second;

	// Ships without an (alive) parent don't follow orders.
	shared_ptr<Ship> parent = ship.GetParent();
	if(!parent)
		return false;
	// If your parent is jumping or absent, that overrides your orders unless
	// your orders are to hold position.
	if(parent && !(shipOrders.Has(Orders::Types::HOLD_POSITION)
		|| shipOrders.Has(Orders::Types::HOLD_ACTIVE) || shipOrders.Has(Orders::Types::MOVE_TO)))
	{
		if(parent->GetSystem() != ship.GetSystem())
			return false;
		if(parent->Commands().Has(Command::JUMP) && ship.JumpsRemaining())
			return false;
	}
	// Do not keep chasing flotsam because another order was given.
	if(ship.GetTargetFlotsam()
		&& (!shipOrders.Has(Orders::Types::HARVEST) || (ship.CanBeCarried() && !ship.HasDeployOrder())))
	{
		ship.SetTargetFlotsam(nullptr);
		return false;
	}

	shared_ptr<Ship> target = shipOrders.GetTargetShip();
	shared_ptr<Minable> targetAsteroid = shipOrders.GetTargetAsteroid();
	const System *targetSystem = shipOrders.GetTargetSystem();
	const Point &targetPoint = shipOrders.GetTargetPoint();
	if(shipOrders.Has(Orders::Types::MOVE_TO) && targetSystem && ship.GetSystem() != targetSystem)
	{
		// The desired position is in a different system. Find the best
		// way to reach that system (via wormhole or jumping). This may
		// result in the ship landing to refuel.
		SelectRoute(ship, targetSystem);

		// Travel there even if your parent is not planning to travel.
		if((ship.GetTargetSystem() && ship.JumpsRemaining()) || ship.GetTargetStellar())
			MoveIndependent(ship, command);
		else
			return false;
	}
	else if((shipOrders.Has(Orders::Types::MOVE_TO) || shipOrders.Has(Orders::Types::HOLD_ACTIVE))
			&& ship.Position().Distance(targetPoint) > 20.)
		MoveTo(ship, command, targetPoint, Point(), 10., .1);
	else if(shipOrders.Has(Orders::Types::HOLD_POSITION) || shipOrders.Has(Orders::Types::HOLD_ACTIVE)
		|| shipOrders.Has(Orders::Types::MOVE_TO))
	{
		if(ship.Velocity().Length() > VELOCITY_ZERO || !ship.GetTargetShip())
			Stop(ship, command);
		else
		{
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
			ship.SetVelocity({0., 0.});
		}
	}
	else if(shipOrders.Has(Orders::Types::MINE) && targetAsteroid)
	{
		ship.SetTargetAsteroid(targetAsteroid);
		// Escorts should chase the player-targeted asteroid.
		MoveToAttack(ship, command, *targetAsteroid);
	}
	else if(shipOrders.Has(Orders::Types::HARVEST))
	{
		if(DoHarvesting(ship, command))
		{
			ship.SetCommands(command);
			ship.SetCommands(firingCommands);
		}
		else
			return false;
	}
	else if(!target)
	{
		// Note: in AI::UpdateKeys() we already made sure that if a set of orders
		// has a target, the target is in-system and targetable. But, to be sure:
		return false;
	}
	else if(shipOrders.Has(Orders::Types::KEEP_STATION))
		KeepStation(ship, command, *target);
	else if(shipOrders.Has(Orders::Types::GATHER))
	{
		if(ship.GetFormationPattern())
			MoveInFormation(ship, command);
		else
			CircleAround(ship, command, *target);
	}
	else
		MoveIndependent(ship, command);

	return true;
}



void AI::MoveInFormation(Ship &ship, Command &command)
{
	shared_ptr<Ship> parent = ship.GetParent();
	if(!parent)
		return;

	const Body *formationLead = parent.get();
	const FormationPattern *pattern = ship.GetFormationPattern();

	// First we retrieve the patterns that are formed around the parent.
	auto &patterns = formations[formationLead];

	// Find the existing FormationPositioner for the pattern, or add one if none exists yet.
	auto insert = patterns.emplace(piecewise_construct,
		forward_as_tuple(pattern),
		forward_as_tuple(formationLead, pattern));

	// Set an iterator to point to the just found or emplaced value.
	auto it = insert.first;

	// Aggressively try to match the position and velocity for the formation position.
	const double POSITION_DEADBAND = ship.Radius() * 1.25;
	constexpr double VELOCITY_DEADBAND = .1;
	bool inPosition = MoveTo(ship, command, it->second.Position(&ship), formationLead->Velocity(), POSITION_DEADBAND,
		VELOCITY_DEADBAND);

	// If we match the position and velocity, then also match the facing angle within some limits.
	constexpr double FACING_TOLERANCE_DEGREES = 3;
	if(inPosition)
	{
		double facingDeltaDegrees = (formationLead->Facing() - ship.Facing()).Degrees();
		if(abs(facingDeltaDegrees) > FACING_TOLERANCE_DEGREES)
			command.SetTurn(facingDeltaDegrees);

		// If the position and velocity matches, smoothly match velocity over multiple frames.
		Point velocityDelta = formationLead->Velocity() - ship.Velocity();
		Point snapAcceleration = velocityDelta.Length() < VELOCITY_ZERO ? velocityDelta : velocityDelta.Unit() * .001;
		if((ship.Velocity() + snapAcceleration).Length() <= ship.MaxVelocity())
			ship.SetVelocity(ship.Velocity() + snapAcceleration);
	}
}



void AI::MoveIndependent(Ship &ship, Command &command)
{
	double invisibleFenceRadius = ship.GetSystem()->InvisibleFenceRadius();

	shared_ptr<const Ship> target = ship.GetTargetShip();
	// NPCs should not be beyond the "fence" unless their target is
	// fairly close to it (or they are intended to be there).
	if(!ship.IsYours() && !ship.GetPersonality().IsUnconstrained())
	{
		if(target)
		{
			Point extrapolated = target->Position() + 120. * (target->Velocity() - ship.Velocity());
			if(extrapolated.Length() >= invisibleFenceRadius)
			{
				MoveTo(ship, command, Point(), Point(), 40., .8);
				if(ship.Velocity().Dot(ship.Position()) > 0.)
					command |= Command::FORWARD;
				return;
			}
		}
		else if(ship.Position().Length() >= invisibleFenceRadius)
		{
			// This ship should not be beyond the fence.
			MoveTo(ship, command, Point(), Point(), 40, .8);
			return;
		}
	}

	bool friendlyOverride = false;
	bool ignoreTargetShip = false;
	if(ship.IsYours())
	{
		auto it = orders.find(&ship);
		if(it != orders.end())
		{
			if(it->second.Has(Orders::Types::MOVE_TO))
				ignoreTargetShip = (ship.GetTargetSystem() && ship.JumpsRemaining()) || ship.GetTargetStellar();
			else if(it->second.Has(Orders::Types::ATTACK) || it->second.Has(Orders::Types::FINISH_OFF))
				friendlyOverride = it->second.GetTargetShip() == target;
		}
	}
	const Government *gov = ship.GetGovernment();
	if(ignoreTargetShip)
	{
		// Do not move to attack, scan, or assist the target ship.
	}
	else if(target && (gov->IsEnemy(target->GetGovernment()) || friendlyOverride))
	{
		bool shouldBoard = ship.Cargo().Free() && ship.GetPersonality().Plunders();
		bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
		if(shouldBoard && target->IsDisabled() && !hasBoarded)
		{
			if(ship.IsBoarding())
				return;
			MoveTo(ship, command, target->Position(), target->Velocity(), 40., .8);
			command |= Command::BOARD;
			boarders[&ship] = target.get();
		}
		else
		{
			Attack(ship, command, *target);
			boarders.erase(&ship);
		}
		return;
	}
	else
	{
		boarders.erase(&ship);
		if(target)
		{
			// An AI ship that is targeting a non-hostile ship should scan it, or move on.
			bool cargoScan = ship.Attributes().Get("cargo scan power");
			bool outfitScan = ship.Attributes().Get("outfit scan power");
			// De-target if the target left my system.
			if(ship.GetSystem() != target->GetSystem())
			{
				target.reset();
				ship.SetTargetShip(nullptr);
			}
			// Detarget if I cannot scan, or if I already scanned the ship.
			else if((!cargoScan || Has(gov, target, ShipEvent::SCAN_CARGO))
					&& (!outfitScan || Has(gov, target, ShipEvent::SCAN_OUTFITS)))
			{
				target.reset();
				ship.SetTargetShip(nullptr);
			}
			// Move to (or near) the ship and scan it.
			else
			{
				if(target->Velocity().Length() > ship.MaxVelocity() * 0.9)
					CircleAround(ship, command, *target);
				else
					MoveTo(ship, command, target->Position(), target->Velocity(), 1., 1.);
				if(!ship.IsYours() && (ship.IsSpecial() || scanPermissions.at(gov)))
					command |= Command::SCAN;
				return;
			}
		}
	}

	// A ship has restricted movement options if it is 'staying', 'lingering', or hostile to its parent.
	const bool shouldStay = StayOrLinger(ship);

	// Ships should choose a random system/planet for travel if they do not
	// already have a system/planet in mind, and are free to move about.
	const System *origin = ship.GetSystem();
	if(!ship.GetTargetSystem() && !ship.GetTargetStellar() && !shouldStay)
	{
		// TODO: This should probably be changed, because JumpsRemaining
		// does not return an accurate number.
		int jumps = ship.JumpsRemaining(false);
		// Each destination system has an average priority of 10.
		// If you only have one jump left, landing should be high priority.
		int planetWeight = jumps ? (1 + 40 / jumps) : 1;

		vector<int> systemWeights;
		int totalWeight = 0;
		const set<const System *> &links = ship.JumpNavigation().HasJumpDrive()
			? origin->JumpNeighbors(ship.JumpNavigation().JumpRange()) : origin->Links();
		if(jumps)
		{
			for(const System *link : links)
			{
				if(ship.IsRestrictedFrom(*link))
				{
					systemWeights.push_back(0);
					continue;
				}
				// Prefer systems in the direction we're facing.
				Point direction = link->Position() - origin->Position();
				int weight = static_cast<int>(
					11. + 10. * ship.Facing().Unit().Dot(direction.Unit()));

				systemWeights.push_back(weight);
				totalWeight += weight;
			}
		}
		int systemTotalWeight = totalWeight;

		// Anywhere you can land that has a port has the same weight. Ships will
		// not land anywhere without a port.
		vector<const StellarObject *> planets;
		for(const StellarObject &object : origin->Objects())
			if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasServices(ship.IsYours())
					&& object.GetPlanet()->CanLand(ship))
			{
				planets.push_back(&object);
				totalWeight += planetWeight;
			}
		// If there are no ports to land on and this ship cannot jump, consider
		// landing on uninhabited planets.
		if(!totalWeight)
			for(const StellarObject &object : origin->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->CanLand(ship))
				{
					planets.push_back(&object);
					totalWeight += planetWeight;
				}
		if(!totalWeight)
		{
			// If there is nothing this ship can land on, have it just go to the
			// star and hover over it rather than drifting far away.
			if(origin->Objects().empty())
				return;
			totalWeight = 1;
			planets.push_back(&origin->Objects().front());
		}

		set<const System *>::const_iterator it = links.begin();
		int choice = Random::Int(totalWeight);
		if(choice < systemTotalWeight)
		{
			for(unsigned i = 0; i < systemWeights.size(); ++i, ++it)
			{
				choice -= systemWeights[i];
				if(choice < 0)
				{
					ship.SetTargetSystem(*it);
					break;
				}
			}
		}
		else
		{
			choice = (choice - systemTotalWeight) / planetWeight;
			ship.SetTargetStellar(planets[choice]);
		}
	}
	// Choose the best method of reaching the target system, which may mean
	// using a local wormhole rather than jumping. If this ship has chosen
	// to land, this decision will not be altered.
	SelectRoute(ship, ship.GetTargetSystem());

	if(ship.GetTargetSystem())
	{
		PrepareForHyperspace(ship, command);
		// Issuing the JUMP command prompts the escorts to get ready to jump.
		command |= Command::JUMP;
		// Issuing the WAIT command will prevent this parent from jumping.
		// When all its non-carried, in-system escorts that are not disabled and
		// have the ability to jump are ready, the WAIT command will be omitted.
		if(!EscortsReadyToJump(ship))
			command |= Command::WAIT;
	}
	else if(ship.GetTargetStellar())
	{
		MoveToPlanet(ship, command);
		if(!shouldStay && ship.Attributes().Get("fuel capacity") && ship.GetTargetStellar()->HasSprite()
				&& ship.GetTargetStellar()->GetPlanet() && ship.GetTargetStellar()->GetPlanet()->CanLand(ship))
			command |= Command::LAND;
		else if(ship.Position().Distance(ship.GetTargetStellar()->Position()) < 100.)
			ship.SetTargetStellar(nullptr);
	}
	else if(shouldStay && !ship.GetSystem()->Objects().empty())
	{
		unsigned i = Random::Int(origin->Objects().size());
		ship.SetTargetStellar(&origin->Objects()[i]);
	}
	// Nowhere to go, and nothing to do, so stay near the system center.
	else if(shouldStay)
		MoveTo(ship, command, Point(), Point(), 40, 0.8);
}



void AI::MoveWithParent(Ship &ship, Command &command, const Ship &parent)
{
	if(ship.GetFormationPattern())
		MoveInFormation(ship, command);
	else
		KeepStation(ship, command, parent);
}



// TODO: Function should be const, but formation flying needed write access to the FormationPositioner.
void AI::MoveEscort(Ship &ship, Command &command)
{
	const Ship &parent = *ship.GetParent();
	const System *currentSystem = ship.GetSystem();
	bool hasFuelCapacity = ship.Attributes().Get("fuel capacity");
	bool needsFuel = ship.NeedsFuel();
	bool isStaying = ship.GetPersonality().IsStaying() || !hasFuelCapacity;
	bool parentIsHere = (currentSystem == parent.GetSystem());
	// Check if the parent already landed, or has a target planet that is in the parent's system.
	const Planet *parentPlanet = (parent.GetPlanet() ? parent.GetPlanet() :
		(parent.GetTargetStellar() ? parent.GetTargetStellar()->GetPlanet() : nullptr));
	bool planetIsHere = (parentPlanet && parentPlanet->IsInSystem(parent.GetSystem()));
	bool systemHasFuel = hasFuelCapacity && currentSystem->HasFuelFor(ship);

	if(parent.Cloaking() == 1 && (ship.GetGovernment() != parent.GetGovernment()))
	{
		if(parent.GetGovernment() && parent.GetGovernment()->IsPlayer() &&
			ship.GetPersonality().IsEscort() && !ship.GetPersonality().IsUninterested())
		{
			// NPCs with the "escort" personality that are not uninterested
			// act as if they were escorts, following the cloaked flagship.
		}
		else
		{
			MoveIndependent(ship, command);
			return;
		}
	}

	// Non-staying escorts should route to their parent ship's system if not already in it.
	if(!parentIsHere && !isStaying)
	{
		if(ship.GetTargetStellar())
		{
			// An escort with an out-of-system parent only lands to
			// refuel or use a wormhole to route toward the parent.
			const Planet *targetPlanet = ship.GetTargetStellar()->GetPlanet();
			if(!targetPlanet || !targetPlanet->CanLand(ship)
					|| !ship.GetTargetStellar()->HasSprite()
					|| (!targetPlanet->IsWormhole() && ship.Fuel() == 1.))
				ship.SetTargetStellar(nullptr);
		}

		// If the ship has no destination or the destination is unreachable, route to the parent's system.
		if(!ship.GetTargetStellar() && (!ship.GetTargetSystem() || !ship.JumpNavigation().JumpFuel(ship.GetTargetSystem())))
		{
			// Route to the parent ship's system and check whether
			// the ship should land (refuel or wormhole) or jump.
			SelectRoute(ship, parent.GetSystem());
		}

		// Perform the action that this ship previously decided on.
		if(ship.GetTargetStellar())
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
		else if(ship.GetTargetSystem() && ship.JumpsRemaining())
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			// If this ship is a parent to members of its fleet,
			// it should wait for them before jumping.
			if(!EscortsReadyToJump(ship))
				command |= Command::WAIT;
		}
		else if(systemHasFuel && ship.Fuel() < 1.)
			// Refuel so that when the parent returns, this ship is ready to rendezvous with it.
			Refuel(ship, command);
		else
			// This ship has no route to the parent's system, so park at the system's center.
			MoveTo(ship, command, Point(), Point(), 40., 0.1);
	}
	// If the parent is in-system and planning to jump, non-staying escorts should follow suit.
	else if(parent.Commands().Has(Command::JUMP) && parent.GetTargetSystem() && !isStaying)
	{
		if(parent.GetTargetSystem() != ship.GetTargetSystem())
			SelectRoute(ship, parent.GetTargetSystem());

		if(ship.GetTargetSystem())
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			if(!(parent.IsEnteringHyperspace() || parent.IsReadyToJump()) || !EscortsReadyToJump(ship))
				command |= Command::WAIT;
		}
		else if(needsFuel && systemHasFuel)
			Refuel(ship, command);
		else if(ship.GetTargetStellar())
		{
			MoveToPlanet(ship, command);
			if(parent.IsEnteringHyperspace())
				command |= Command::LAND;
		}
		else if(needsFuel)
			// Return to the system center to maximize solar collection rate.
			MoveTo(ship, command, Point(), Point(), 40., 0.1);
		else
			// This ship has no route to the parent's destination system, so protect it until it jumps away.
			KeepStation(ship, command, parent);
	}
	// If an escort is out of fuel, they should refuel without waiting for the
	// "parent" to land (because the parent may not be planning on landing).
	else if(systemHasFuel && needsFuel)
		Refuel(ship, command);
	else if((parent.Commands().Has(Command::LAND) || parent.IsLanding()) && parentIsHere && planetIsHere)
	{
		if(parentPlanet->CanLand(ship))
		{
			ship.SetTargetSystem(nullptr);
			ship.SetTargetStellar(parent.GetTargetStellar());
			if(parent.IsLanding())
			{
				MoveToPlanet(ship, command);
				command |= Command::LAND;
			}
			else
				MoveWithParent(ship, command, parent);
		}
		else if(parentPlanet->IsWormhole())
		{
			const auto *wormhole = parentPlanet->GetWormhole();
			SelectRoute(ship, &wormhole->WormholeDestination(*currentSystem));

			if(ship.GetTargetSystem())
			{
				PrepareForHyperspace(ship, command);
				if(parent.IsLanding())
					command |= Command::JUMP;
			}
			else if(ship.GetTargetStellar())
			{
				MoveToPlanet(ship, command);
				if(parent.IsLanding())
					command |= Command::LAND;
			}
			else if(needsFuel)
				// Return to the system center to maximize solar collection rate.
				MoveTo(ship, command, Point(), Point(), 40., 0.1);
			else
				// This ship has no route to the parent's destination system, so protect it until it jumps away.
				MoveWithParent(ship, command, parent);
		}
		else
			MoveWithParent(ship, command, parent);
	}
	else if(parent.Commands().Has(Command::BOARD) && parent.GetTargetShip().get() == &ship)
		Stop(ship, command, .2);
	else
		MoveWithParent(ship, command, parent);
}



// Prefer your parent's target planet for refueling, but if it and your current
// target planet can't fuel you, try to find one that can.
void AI::Refuel(Ship &ship, Command &command)
{
	const StellarObject *parentTarget = (ship.GetParent() ? ship.GetParent()->GetTargetStellar() : nullptr);
	if(CanRefuel(ship, parentTarget))
		ship.SetTargetStellar(parentTarget);
	else if(!CanRefuel(ship, ship.GetTargetStellar()))
		ship.SetTargetStellar(FindLandingLocation(ship));

	if(ship.GetTargetStellar())
	{
		MoveToPlanet(ship, command);
		command |= Command::LAND;
	}
}



bool AI::CanRefuel(const Ship &ship, const StellarObject *target)
{
	if(!target)
		return false;

	const Planet *planet = target->GetPlanet();
	if(!planet)
		return false;

	if(!planet->IsInSystem(ship.GetSystem()))
		return false;

	if(!planet->HasFuelFor(ship))
		return false;

	return true;
}



// Set the ship's target system or planet in order to reach the
// next desired system. Will target a landable planet to refuel.
// If the ship is an escort it will only use routes known to the player.
void AI::SelectRoute(Ship &ship, const System *targetSystem)
{
	const System *from = ship.GetSystem();
	if(from == targetSystem || !targetSystem)
		return;
	RoutePlan route = GetRoutePlan(ship, targetSystem);
	if(ShouldRefuel(ship, route))
	{
		// There is at least one planet that can refuel the ship.
		ship.SetTargetStellar(AI::FindLandingLocation(ship));
		return;
	}
	const System *nextSystem = route.FirstStep();
	// The destination may be accessible by both jump and wormhole.
	// Prefer wormhole travel in these cases, to conserve fuel.
	if(nextSystem)
		for(const StellarObject &object : from->Objects())
		{
			if(!object.HasSprite() || !object.HasValidPlanet())
				continue;

			const Planet &planet = *object.GetPlanet();
			if(planet.IsWormhole() && planet.IsAccessible(&ship)
				&& &planet.GetWormhole()->WormholeDestination(*from) == nextSystem)
			{
				ship.SetTargetStellar(&object);
				ship.SetTargetSystem(nullptr);
				return;
			}
		}
	// Either there is no viable wormhole route to this system, or
	// the target system cannot be reached.
	ship.SetTargetSystem(nextSystem);
	ship.SetTargetStellar(nullptr);
}



// Determine if a carried ship meets any of the criteria for returning to its parent.
bool AI::ShouldDock(const Ship &ship, const Ship &parent, const System *playerSystem) const
{
	// If your parent is disabled, you should not attempt to board it.
	// (Doing so during combat will likely lead to its destruction.)
	if(parent.IsDisabled())
		return false;

	// A player-owned carried ship should return to its carrier when the player
	// has ordered it to "no longer deploy" or when it is not in the current system.
	// A non-player-owned carried ship should retreat if its parent is calling it back.
	if(ship.IsYours())
	{
		if(!ship.HasDeployOrder() || ship.GetSystem() != playerSystem)
			return true;
	}
	else if(!parent.Commands().Has(Command::DEPLOY))
		return true;

	// If a carried ship has repair abilities, avoid having it get stuck oscillating between
	// retreating and attacking when at exactly 50% health by adding hysteresis to the check.
	double minHealth = RETREAT_HEALTH + .25 + .25 * !ship.Commands().Has(Command::DEPLOY);
	if(ship.Health() < minHealth && (!ship.IsYours() || Preferences::Has("Damaged fighters retreat")))
		return true;

	// If a fighter is armed with only ammo-using weapons, but no longer has the ammunition
	// needed to use them, it should dock if the parent can supply that ammo.
	auto requiredAmmo = set<const Outfit *>{};
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(weapon && !hardpoint.IsSpecial())
		{
			const Outfit *ammo = weapon->Ammo();
			if(!ammo || ship.OutfitCount(ammo))
			{
				// This fighter has at least one usable weapon, and
				// thus does not need to dock to continue fighting.
				requiredAmmo.clear();
				break;
			}
			else if(parent.OutfitCount(ammo))
				requiredAmmo.insert(ammo);
		}
	}
	if(!requiredAmmo.empty())
		return true;

	// If a carried ship has fuel capacity but is very low, it should return if
	// the parent can refuel it.
	double maxFuel = ship.Attributes().Get("fuel capacity");
	if(maxFuel && ship.Fuel() < .005 && parent.JumpNavigation().JumpFuel() < parent.Fuel() *
			parent.Attributes().Get("fuel capacity") - maxFuel)
		return true;

	// NPC ships should always transfer cargo. Player ships should only
	// transfer cargo if the player has the AI preference set for it.
	if(!ship.IsYours() || Preferences::Has("Fighters transfer cargo"))
	{
		// If an out-of-combat carried ship is carrying a significant cargo
		// load and can transfer some of it to the parent, it should do so.
		bool hasEnemy = ship.GetTargetShip() && ship.GetTargetShip()->GetGovernment()->IsEnemy(ship.GetGovernment());
		if(!hasEnemy && parent.Cargo().Free())
		{
			const CargoHold &cargo = ship.Cargo();
			// Mining ships only mine while they have 5 or more free space. While mining, carried ships
			// do not consider docking unless their parent is far from a targetable asteroid.
			if(!cargo.IsEmpty() && cargo.Size() && cargo.Free() < 5)
				return true;
		}
	}

	return false;
}



double AI::TurnBackward(const Ship &ship)
{
	return TurnToward(ship, -ship.Velocity());
}



// Determine the value to use in Command::SetTurn() to turn the ship towards the desired facing.
// "precision" is an optional argument corresponding to a value of the dot product of the current and target facing
// vectors above which no turning should be attempting, to reduce constant, minute corrections.
double AI::TurnToward(const Ship &ship, const Point &vector, const double precision)
{
	Point facing = ship.Facing().Unit();
	double cross = vector.Cross(facing);

	double dot = vector.Dot(facing);
	if(dot > 0.)
	{
		// Is the facing direction aligned with the target direction with sufficient precision?
		// The maximum angle between the two directions is given by: arccos(sqrt(precision)).
		bool close = false;
		if(precision < 1. && precision > 0. && dot * dot >= precision * vector.LengthSquared())
			close = true;
		double angle = asin(min(1., max(-1., cross / vector.Length()))) * TO_DEG;
		// Is the angle between the facing and target direction smaller than
		// the angle the ship can turn through in one step?
		if(fabs(angle) < ship.TurnRate())
		{
			// If the ship is within one step of the target direction,
			// and the facing is already sufficiently aligned with the target direction,
			// don't turn any further.
			if(close)
				return 0.;
			return -angle / ship.TurnRate();
		}
	}

	bool left = cross < 0.;
	return left - !left;
}



bool AI::MoveToPlanet(const Ship &ship, Command &command, double cruiseSpeed)
{
	if(!ship.GetTargetStellar())
		return false;

	const Point &target = ship.GetTargetStellar()->Position();
	return MoveTo(ship, command, target, Point(), ship.GetTargetStellar()->Radius(), 1., cruiseSpeed);
}



// Instead of moving to a point with a fixed location, move to a moving point (Ship = position + velocity)
bool AI::MoveTo(const Ship &ship, Command &command, const Point &targetPosition,
	const Point &targetVelocity, double radius, double slow, double cruiseSpeed)
{
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	Point dp = targetPosition - position;
	Point dv = targetVelocity - velocity;

	double speed = dv.Length();

	bool isClose = (dp.Length() < radius);
	if(isClose && speed < slow)
		return true;

	bool shouldReverse = false;
	dp = targetPosition - StoppingPoint(ship, targetVelocity, shouldReverse);

	// Calculate target vector required to get where we want to be.
	Point tv = dp;
	bool hasCruiseSpeed = (cruiseSpeed > 0.);
	if(hasCruiseSpeed)
	{
		// The ship prefers a velocity at cruise-speed towards the target, so we need
		// to compare this preferred velocity to the current velocity and apply the
		// delta to get to the preferred velocity.
		tv = (dp.Unit() * cruiseSpeed) - velocity;
		// If we are moving close to our preferred velocity, then face towards the target.
		if(tv.LengthSquared() < .01)
			tv = dp;
	}

	bool isFacing = (tv.Unit().Dot(angle.Unit()) > .95);
	if(!isClose || (!isFacing && !shouldReverse))
		command.SetTurn(TurnToward(ship, tv));

	// Drag is not applied when not thrusting, so stop thrusting when close to max speed
	// to save energy. Work with a slightly lower maximum velocity to avoid border cases.
	// In order for a ship to use their afterburner, they must also have the forward
	// command active. Therefore, if this ship should use its afterburner, use the
	// max velocity with afterburner thrust included.
	double maxVelocity = ship.MaxVelocity(ShouldUseAfterburner(ship)) * .99;
	if(isFacing && (velocity.LengthSquared() <= maxVelocity * maxVelocity
			|| dp.Unit().Dot(velocity.Unit()) < .95))
	{
		// We set full forward power when we don't have a cruise-speed, when we are below
		// cruise-speed or when we need to do course corrections.
		bool movingTowardsTarget = (velocity.Unit().Dot(dp.Unit()) > .95);
		if(!hasCruiseSpeed || !movingTowardsTarget || velocity.Length() < cruiseSpeed)
			command |= Command::FORWARD;
	}
	else if(shouldReverse)
	{
		command.SetTurn(TurnToward(ship, velocity));
		command |= Command::BACK;
	}

	return false;
}



bool AI::Stop(const Ship &ship, Command &command, double maxSpeed, const Point &direction)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();

	double speed = velocity.Length();

	// If asked for a complete stop, the ship needs to be going much slower.
	if(speed <= (maxSpeed ? maxSpeed : VELOCITY_ZERO))
		return true;
	if(!maxSpeed)
		command |= Command::STOP;

	// If you're moving slow enough that one frame of acceleration could bring
	// you to a stop, make sure you're pointed perfectly in the right direction.
	// This is a fudge factor for how straight you must be facing: it increases
	// from 0.8 when it will take many frames to stop, to nearly 1 when it will
	// take less than 1 frame to stop.
	double stopTime = speed / ship.Acceleration();
	double limit = .8 + .2 / (1. + stopTime * stopTime * stopTime * .001);

	// If you have a reverse thruster, figure out whether using it is faster
	// than turning around and using your main thruster.
	if(ship.Attributes().Get("reverse thrust"))
	{
		// Figure out your stopping time using your main engine:
		double degreesToTurn = TO_DEG * acos(min(1., max(-1., -velocity.Unit().Dot(angle.Unit()))));
		double forwardTime = degreesToTurn / ship.TurnRate();
		forwardTime += stopTime;

		// Figure out your reverse thruster stopping time:
		double reverseTime = (180. - degreesToTurn) / ship.TurnRate();
		reverseTime += speed / ship.ReverseAcceleration();

		// If you want to end up facing a specific direction, add the extra turning time.
		if(direction)
		{
			// Time to turn from facing backwards to target:
			double degreesFromBackwards = TO_DEG * acos(min(1., max(-1., direction.Unit().Dot(-velocity.Unit()))));
			double turnFromBackwardsTime = degreesFromBackwards / ship.TurnRate();
			forwardTime += turnFromBackwardsTime;

			// Time to turn from facing forwards to target:
			double degreesFromForward = TO_DEG * acos(min(1., max(-1., direction.Unit().Dot(angle.Unit()))));
			double turnFromForwardTime = degreesFromForward / ship.TurnRate();
			reverseTime += turnFromForwardTime;
		}

		if(reverseTime < forwardTime)
		{
			command.SetTurn(TurnToward(ship, velocity));
			if(velocity.Unit().Dot(angle.Unit()) > limit)
				command |= Command::BACK;
			return false;
		}
	}

	command.SetTurn(TurnBackward(ship));
	if(velocity.Unit().Dot(angle.Unit()) < -limit)
		command |= Command::FORWARD;

	return false;
}



void AI::PrepareForHyperspace(const Ship &ship, Command &command)
{
	bool hasHyperdrive = ship.JumpNavigation().HasHyperdrive();
	double scramThreshold = ship.Attributes().Get("scram drive");
	bool hasJumpDrive = ship.JumpNavigation().HasJumpDrive();
	if(!hasHyperdrive && !hasJumpDrive)
		return;

	bool isJump = (ship.JumpNavigation().GetCheapestJumpType(ship.GetTargetSystem()).first == JumpType::JUMP_DRIVE);

	Point direction = ship.GetTargetSystem()->Position() - ship.GetSystem()->Position();
	double departure = isJump ?
		ship.GetSystem()->JumpDepartureDistance() :
		ship.GetSystem()->HyperDepartureDistance();
	double squaredDeparture = departure * departure + SAFETY_OFFSET;
	if(ship.Position().LengthSquared() < squaredDeparture)
	{
		Point closestDeparturePoint = ship.Position().Unit() * (departure + SAFETY_OFFSET);
		MoveTo(ship, command, closestDeparturePoint, Point(), 0., 0.);
	}
	else if(!isJump && scramThreshold)
	{
		direction = direction.Unit();
		Point normal(-direction.Y(), direction.X());

		double deviation = ship.Velocity().Dot(normal);
		if(fabs(deviation) > scramThreshold)
		{
			// Need to maneuver; not ready to jump
			if((ship.Facing().Unit().Dot(normal) < 0) == (deviation < 0))
				// Thrusting from this angle is counterproductive
				direction = -deviation * normal;
			else
			{
				command |= Command::FORWARD;

				// How much correction will be applied to deviation by thrusting
				// as I turn back toward the jump direction.
				double turnRateRadians = ship.TurnRate() * TO_RAD;
				double cos = ship.Facing().Unit().Dot(direction);
				// integral(t*sin(r*x), angle/r, 0) = t/r * (1 - cos(angle)), so:
				double correctionWhileTurning = fabs(1 - cos) * ship.Acceleration() / turnRateRadians;
				// (Note that this will always underestimate because thrust happens before turn)

				if(fabs(deviation) - correctionWhileTurning > scramThreshold)
					// Want to thrust from an even sharper angle
					direction = -deviation * normal;
			}
		}
		command.SetTurn(TurnToward(ship, direction));
	}
	// If we're a jump drive, just stop.
	else if(isJump)
		Stop(ship, command, ship.Attributes().Get("jump speed"));
	// Else stop in the fastest way to end facing in the right direction
	else if(Stop(ship, command, ship.Attributes().Get("jump speed"), direction))
		command.SetTurn(TurnToward(ship, direction));
}



void AI::CircleAround(const Ship &ship, Command &command, const Body &target)
{
	Point direction = target.Position() - ship.Position();
	command.SetTurn(TurnToward(ship, direction));

	double length = direction.Length();
	if(length > 200. && ship.Facing().Unit().Dot(direction) >= 0.)
	{
		command |= Command::FORWARD;

		// If the ship is far away enough the ship should use the afterburner.
		if(length > 750. && ShouldUseAfterburner(ship))
			command |= Command::AFTERBURNER;
	}
}



void AI::Swarm(const Ship &ship, Command &command, const Body &target)
{
	Point direction = target.Position() - ship.Position();
	double maxSpeed = ship.MaxVelocity();
	double rendezvousTime = RendezvousTime(direction, target.Velocity(), maxSpeed);
	if(std::isnan(rendezvousTime) || rendezvousTime > 600.)
		rendezvousTime = 600.;
	direction += rendezvousTime * target.Velocity();
	MoveTo(ship, command, target.Position() + direction, .5 * maxSpeed * direction.Unit(), 50., 2.);
}



void AI::KeepStation(const Ship &ship, Command &command, const Body &target)
{
	// Constants:
	static const double MAX_TIME = 600.;
	static const double LEAD_TIME = 500.;
	static const double POSITION_DEADBAND = 200.;
	static const double VELOCITY_DEADBAND = 1.5;
	static const double TIME_DEADBAND = 120.;
	static const double THRUST_DEADBAND = .5;

	// Current properties of the two ships:
	double maxV = ship.MaxVelocity();
	double accel = ship.Acceleration();
	double turn = ship.TurnRate();
	double mass = ship.InertialMass();
	Point unit = ship.Facing().Unit();
	double currentAngle = ship.Facing().Degrees();
	// This is where we want to be relative to where we are now:
	Point velocityDelta = target.Velocity() - ship.Velocity();
	Point positionDelta = target.Position() + LEAD_TIME * velocityDelta - ship.Position();
	double positionSize = positionDelta.Length();
	double positionWeight = positionSize / (positionSize + POSITION_DEADBAND);
	// This is how fast we want to be going relative to how fast we're going now:
	velocityDelta -= unit * VELOCITY_DEADBAND;
	double velocitySize = velocityDelta.Length();
	double velocityWeight = velocitySize / (velocitySize + VELOCITY_DEADBAND);

	// Time it will take (roughly) to move to the target ship:
	double positionTime = RendezvousTime(positionDelta, target.Velocity(), maxV);
	if(std::isnan(positionTime) || positionTime > MAX_TIME)
		positionTime = MAX_TIME;
	Point rendezvous = positionDelta + target.Velocity() * positionTime;
	double positionAngle = Angle(rendezvous).Degrees();
	positionTime += AngleDiff(currentAngle, positionAngle) / turn;
	positionTime += (rendezvous.Unit() * maxV - ship.Velocity()).Length() / accel;
	// If you are very close, stop trying to adjust:
	positionTime *= positionWeight * positionWeight;

	// Time it will take (roughly) to adjust your velocity to match the target:
	double velocityTime = velocityDelta.Length() / accel;
	double velocityAngle = Angle(velocityDelta).Degrees();
	velocityTime += AngleDiff(currentAngle, velocityAngle) / turn;
	// If you are very close, stop trying to adjust:
	velocityTime *= velocityWeight * velocityWeight;

	// Focus on matching position or velocity depending on which will take longer.
	double totalTime = positionTime + velocityTime + TIME_DEADBAND;
	positionWeight = positionTime / totalTime;
	velocityWeight = velocityTime / totalTime;
	double facingWeight = TIME_DEADBAND / totalTime;

	// Determine the angle we want to face, interpolating smoothly between three options.
	Point facingGoal = rendezvous.Unit() * positionWeight
		+ velocityDelta.Unit() * velocityWeight
		+ target.Facing().Unit() * facingWeight;
	double targetAngle = Angle(facingGoal).Degrees() - currentAngle;
	if(abs(targetAngle) > 180.)
		targetAngle += (targetAngle < 0. ? 360. : -360.);
	// Avoid "turn jitter" when position & velocity are well-matched.
	bool changedDirection = (signbit(ship.Commands().Turn()) != signbit(targetAngle));
	double targetTurn = abs(targetAngle / turn);
	double lastTurn = abs(ship.Commands().Turn());
	if(lastTurn && (changedDirection || (lastTurn < 1. && targetTurn > lastTurn)))
	{
		// Keep the desired turn direction, but damp the per-frame turn rate increase.
		double dampedTurn = (changedDirection ? 0. : lastTurn) + min(.025, targetTurn);
		command.SetTurn(copysign(dampedTurn, targetAngle));
	}
	else if(targetTurn < 1.)
		command.SetTurn(copysign(targetTurn, targetAngle));
	else
		command.SetTurn(targetAngle);

	// Determine whether to apply thrust.
	Point drag = ship.Velocity() * ship.DragForce();
	if(ship.Attributes().Get("reverse thrust"))
	{
		// Don't take drag into account when reverse thrusting, because this
		// estimate of how it will be applied can be quite inaccurate.
		Point a = (unit * (-ship.Attributes().Get("reverse thrust") / mass)).Unit();
		double direction = positionWeight * positionDelta.Dot(a) / POSITION_DEADBAND
			+ velocityWeight * velocityDelta.Dot(a) / VELOCITY_DEADBAND;
		if(direction > THRUST_DEADBAND)
		{
			command |= Command::BACK;
			return;
		}
	}
	Point a = (unit * accel - drag).Unit();
	double direction = positionWeight * positionDelta.Dot(a) / POSITION_DEADBAND
		+ velocityWeight * velocityDelta.Dot(a) / VELOCITY_DEADBAND;
	if(direction > THRUST_DEADBAND)
		command |= Command::FORWARD;
}



void AI::Attack(const Ship &ship, Command &command, const Ship &target)
{
	// Deploy any fighters you are carrying.
	if(!ship.IsYours() && ship.HasBays())
	{
		command |= Command::DEPLOY;
		Deploy(ship, false);
	}
	// Ramming AI doesn't take weapon range or self-damage into account, instead opting to bum-rush the target.
	if(ship.GetPersonality().IsRamming())
	{
		MoveToAttack(ship, command, target);
		return;
	}

	// Check if this ship is fast enough to keep distance from target.
	// Have a 10% minimum to avoid ships getting in a chase loop.
	const bool isAbleToRun = target.MaxVelocity() * SAFETY_MULTIPLIER < ship.MaxVelocity();

	const ShipAICache &shipAICache = ship.GetAICache();
	const bool useArtilleryAI = shipAICache.IsArtilleryAI() && isAbleToRun;
	const double shortestRange = shipAICache.ShortestRange();
	const double shortestArtillery = shipAICache.ShortestArtillery();
	double minSafeDistance = isAbleToRun ? shipAICache.MinSafeDistance() : 0.;

	const double totalRadius = ship.Radius() + target.Radius();
	const Point direction = target.Position() - ship.Position();
	// Average distance from this ship's weapons to the enemy ship.
	const double weaponDistanceFromTarget = direction.Length() - totalRadius / 3.;

	// If this ship has mostly long-range weapons, or some weapons have a
	// blast radius, it should keep some distance instead of closing in.
	// If a weapon has blast radius, some leeway helps avoid getting hurt.
	if(minSafeDistance || (useArtilleryAI && shortestRange < weaponDistanceFromTarget))
	{
		minSafeDistance = 1.25 * minSafeDistance + totalRadius;

		double approachSpeed = (ship.Velocity() - target.Velocity()).Dot(direction.Unit());
		double slowdownDistance = 0.;
		// If this ship can use reverse thrusters, consider doing so.
		double reverseSpeed = ship.MaxReverseVelocity();
		bool useReverse = reverseSpeed && (reverseSpeed >= min(target.MaxVelocity(), ship.MaxVelocity())
				|| target.Velocity().Dot(-direction.Unit()) <= reverseSpeed);
		slowdownDistance = approachSpeed * approachSpeed / (useReverse ?
			ship.ReverseAcceleration() : (ship.Acceleration() + 160. / ship.TurnRate())) / 2.;

		// If we're too close, run away.
		if(direction.Length() <
				max(minSafeDistance + max(slowdownDistance, 0.), useArtilleryAI * .75 * shortestArtillery))
		{
			if(useReverse)
			{
				command.SetTurn(TurnToward(ship, direction));
				if(ship.Facing().Unit().Dot(direction) >= 0.)
					command |= Command::BACK;
			}
			else
			{
				command.SetTurn(TurnToward(ship, -direction));
				if(ship.Facing().Unit().Dot(direction) <= 0.)
					command |= Command::FORWARD;
			}
		}
		else
		{
			// This isn't perfect, but it works well enough.
			if((useArtilleryAI && (approachSpeed > 0. && weaponDistanceFromTarget < shortestArtillery * .9)) ||
					weaponDistanceFromTarget < shortestRange * .75)
				AimToAttack(ship, command, target);
			else
				MoveToAttack(ship, command, target);
		}
	}
	// Fire if we can or move closer to use all weapons.
	else
		if(weaponDistanceFromTarget < shortestRange * .75)
			AimToAttack(ship, command, target);
		else
			MoveToAttack(ship, command, target);
}



void AI::AimToAttack(const Ship &ship, Command &command, const Body &target)
{
	command.SetTurn(TurnToward(ship, TargetAim(ship, target)));
}



void AI::MoveToAttack(const Ship &ship, Command &command, const Body &target)
{
	Point direction = target.Position() - ship.Position();

	// First of all, aim in the direction that will hit this target.
	AimToAttack(ship, command, target);

	// Calculate this ship's "turning radius"; that is, the smallest circle it
	// can make while at its current speed.
	double stepsInFullTurn = 360. / ship.TurnRate();
	double circumference = stepsInFullTurn * ship.Velocity().Length();
	double diameter = max(200., circumference / PI);

	const auto facing = ship.Facing().Unit().Dot(direction.Unit());
	// If the ship has reverse thrusters and the target is behind it, we can
	// use them to reach the target more quickly.
	if(facing < -.75 && ship.Attributes().Get("reverse thrust"))
		command |= Command::BACK;
	// Only apply thrust if either:
	// This ship is within 90 degrees of facing towards its target and far enough away not to overshoot
	// if it accelerates while needing to turn further, or:
	// This ship is moving away from its target but facing mostly towards it.
	else if((facing >= 0. && direction.Length() > diameter)
			|| (ship.Velocity().Dot(direction) < 0. && facing >= .9))
	{
		command |= Command::FORWARD;
		// Use afterburner, if applicable.
		if(direction.Length() > 600. && ShouldUseAfterburner(ship))
			command |= Command::AFTERBURNER;
	}
}



void AI::PickUp(const Ship &ship, Command &command, const Body &target)
{
	// Figure out the target's velocity relative to the ship.
	Point p = target.Position() - ship.Position();
	Point v = target.Velocity() - ship.Velocity();
	double vMax = ship.MaxVelocity();

	// Estimate where the target will be by the time we reach it.
	double time = RendezvousTime(p, v, vMax);
	if(std::isnan(time))
		time = p.Length() / vMax;
	double degreesToTurn = TO_DEG * acos(min(1., max(-1., p.Unit().Dot(ship.Facing().Unit()))));
	time += degreesToTurn / ship.TurnRate();
	p += v * time;

	// Move toward the target.
	command.SetTurn(TurnToward(ship, p));
	double dp = p.Unit().Dot(ship.Facing().Unit());
	if(dp > .7)
		command |= Command::FORWARD;

	// Use the afterburner if it will not cause you to miss your target.
	double squareDistance = p.LengthSquared();
	if(command.Has(Command::FORWARD) && ShouldUseAfterburner(ship))
		if(dp > max(.9, min(.9999, 1. - squareDistance / 10000000.)))
			command |= Command::AFTERBURNER;
}



// Determine if using an afterburner does not use up reserve fuel, cause undue
// energy strain, or undue thermal loads if almost overheated.
bool AI::ShouldUseAfterburner(const Ship &ship)
{
	if(!ship.Attributes().Get("afterburner thrust"))
		return false;

	double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
	double neededFuel = ship.Attributes().Get("afterburner fuel");
	double energy = ship.Energy() * ship.Attributes().Get("energy capacity");
	double neededEnergy = ship.Attributes().Get("afterburner energy");
	if(energy == 0.)
		energy = ship.Attributes().Get("energy generation")
				+ 0.2 * ship.Attributes().Get("solar collection")
				- ship.Attributes().Get("energy consumption");
	double outputHeat = ship.Attributes().Get("afterburner heat") / (100 * ship.Mass());
	if((!neededFuel || fuel - neededFuel > ship.JumpNavigation().JumpFuel())
			&& (!neededEnergy || neededEnergy / energy < 0.25)
			&& (!outputHeat || ship.Heat() + outputHeat < .9))
		return true;

	return false;
}



// "Appeasing" ships will dump cargo after being injured, if they are being targeted.
void AI::DoAppeasing(const shared_ptr<Ship> &ship, double *threshold) const
{
	double health = .5 * ship->Shields() + ship->Hull();
	if(1. - health <= *threshold)
		return;

	const auto enemies = GetShipsList(*ship, true);
	if(none_of(enemies.begin(), enemies.end(), [&ship](const Ship *foe) noexcept -> bool
			{ return !foe->IsDisabled() && foe->GetTargetShip() == ship; }))
		return;

	int toDump = 11 + (1. - health) * .5 * ship->Cargo().Size();
	for(auto &&commodity : ship->Cargo().Commodities())
		if(commodity.second && toDump > 0)
		{
			int dumped = min(commodity.second, toDump);
			ship->Jettison(commodity.first, dumped, true);
			toDump -= dumped;
		}

	*threshold = (1. - health) + .1;

	if(ship->GetPersonality().IsMute())
		return;
	const Government *government = ship->GetGovernment();
	const string &language = government->Language();
	if(language.empty() || player.Conditions().Get("language: " + language))
		Messages::Add({government->DisplayName() + " " + ship->Noun() + " \"" + ship->GivenName()
			+ "\": Please, just take my cargo and leave me alone.",
			GameData::MessageCategories().Get("low")});

}



// Find a target ship to flock around at high speed.
void AI::DoSwarming(Ship &ship, Command &command, shared_ptr<Ship> &target)
{
	// Find a new ship to target on average every 10 seconds, or if the current target
	// is no longer eligible. If landing, release the old target so others can swarm it.
	if(ship.IsLanding() || !target || !CanSwarm(ship, *target) || !Random::Int(600))
	{
		if(target)
		{
			// Allow another swarming ship to consider the target.
			auto sit = swarmCount.find(target.get());
			if(sit != swarmCount.end() && sit->second > 0)
				--sit->second;
			// Release the current target.
			target.reset();
			ship.SetTargetShip(target);
		}
		// If here just because we are about to land, do not seek a new target.
		if(ship.IsLanding())
			return;

		int lowestCount = 7;
		// Consider swarming around non-hostile ships in the same system.
		const auto others = GetShipsList(ship, false);
		for(auto *other : others)
			if(!other->GetPersonality().IsSwarming())
			{
				// Prefer to swarm ships that are not already being heavily swarmed.
				int count = swarmCount[other] + Random::Int(4);
				if(count < lowestCount)
				{
					target = other->shared_from_this();
					lowestCount = count;
				}
			}
		ship.SetTargetShip(target);
		if(target)
			++swarmCount[target.get()];
	}
	// If a friendly ship to flock with was not found, return to an available planet.
	if(target)
		Swarm(ship, command, *target);
	else if(ship.Zoom() == 1.)
		Refuel(ship, command);
}



void AI::DoSurveillance(Ship &ship, Command &command, shared_ptr<Ship> &target)
{
	const bool isStaying = ship.GetPersonality().IsStaying();
	// Since DoSurveillance is called after target-seeking and firing, if this
	// ship has a target, that target is guaranteed to be targetable.
	if(target && (target->GetSystem() != ship.GetSystem() || target->IsEnteringHyperspace()))
	{
		target.reset();
		ship.SetTargetShip(target);
	}
	// If you have a hostile target, pursuing and destroying it has priority.
	if(target && ship.GetGovernment()->IsEnemy(target->GetGovernment()))
	{
		// Automatic aiming and firing already occurred.
		MoveIndependent(ship, command);
		return;
	}

	// Choose a surveillance behavior.
	if(ship.GetTargetSystem())
	{
		// Unload surveillance drones in this system before leaving.
		if(!isStaying)
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
		}
		if(ship.HasBays())
		{
			command |= Command::DEPLOY;
			Deploy(ship, false);
		}
	}
	else if(ship.GetTargetStellar())
	{
		// Approach the planet and "land" on it (i.e. scan it).
		MoveToPlanet(ship, command);
		double atmosphereScan = ship.Attributes().Get("atmosphere scan");
		double distance = ship.Position().Distance(ship.GetTargetStellar()->Position());
		if(distance < atmosphereScan && !Random::Int(100))
			ship.SetTargetStellar(nullptr);
		else if(!isStaying)
			command |= Command::LAND;
	}
	else if(target)
	{
		// Approach and scan the targeted, friendly ship's cargo or outfits.
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		// If the pointer to the target ship exists, it is targetable and in-system.
		const Government *gov = ship.GetGovernment();
		bool mustScanCargo = cargoScan && !Has(gov, target, ShipEvent::SCAN_CARGO);
		bool mustScanOutfits = outfitScan && !Has(gov, target, ShipEvent::SCAN_OUTFITS);
		if(!mustScanCargo && !mustScanOutfits)
			ship.SetTargetShip(shared_ptr<Ship>());
		else
		{
			if(target->Velocity().Length() > ship.MaxVelocity() * 0.9)
				CircleAround(ship, command, *target);
			else
				MoveTo(ship, command, target->Position(), target->Velocity(), 1., 1.);
			command |= Command::SCAN;
		}
	}
	else
	{
		const System *system = ship.GetSystem();
		const Government *gov = ship.GetGovernment();

		// Consider scanning any non-hostile ship in this system that your government hasn't scanned.
		// A surveillance ship may only make up to 12 successful scans (6 ships scanned
		// if the ship is using both scanners) and spend up to 5 minutes searching for
		// scan targets. After that, stop scanning ship targets. This is so that scanning
		// ships in high spawn rate systems don't build up over time, as they always have
		// a new ship they can try to scan.
		vector<Ship *> targetShips;
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		auto cargoScansIt = cargoScans.find(&ship);
		auto outfitScansIt = outfitScans.find(&ship);
		auto scanTimeIt = scanTime.find(&ship);
		int shipScanCount = cargoScansIt != cargoScans.end() ? cargoScansIt->second.size() : 0;
		shipScanCount += outfitScansIt != outfitScans.end() ? outfitScansIt->second.size() : 0;
		int shipScanTime = scanTimeIt != scanTime.end() ? scanTimeIt->second : 0;
		if((cargoScan || outfitScan) && shipScanCount < 12 && shipScanTime < 18000)
		{
			for(const auto &it : GetShipsList(ship, false))
				if(it->GetGovernment() != gov)
				{
					auto ptr = it->shared_from_this();
					if((!cargoScan || Has(gov, ptr, ShipEvent::SCAN_CARGO))
							&& (!outfitScan || Has(gov, ptr, ShipEvent::SCAN_OUTFITS)))
						continue;

					if(it->IsTargetable())
						targetShips.emplace_back(it);
				}
		}

		// Consider scanning any planetary object in the system, if able.
		vector<const StellarObject *> targetPlanets;
		double atmosphereScan = ship.Attributes().Get("atmosphere scan");
		if(atmosphereScan)
			for(const StellarObject &object : system->Objects())
				if(object.HasSprite() && !object.IsStar() && !object.IsStation())
					targetPlanets.push_back(&object);

		// If this ship can jump away, consider traveling to a nearby system.
		vector<const System *> targetSystems;
		// TODO: These ships cannot travel through wormholes?
		if(ship.JumpsRemaining(false))
		{
			const auto &links = ship.JumpNavigation().HasJumpDrive() ?
				system->JumpNeighbors(ship.JumpNavigation().JumpRange()) : system->Links();
			for(const System *link : links)
				if(!ship.IsRestrictedFrom(*link))
					targetSystems.push_back(link);
		}

		unsigned total = targetShips.size() + targetPlanets.size() + targetSystems.size();
		// If there is nothing for this ship to scan, have it patrol the entire system
		// instead of drifting or stopping.
		if(!total)
		{
			DoPatrol(ship, command);
			return;
		}
		// Pick one of the valid surveillance targets at random to focus on.
		unsigned index = Random::Int(total);
		if(index < targetShips.size())
			ship.SetTargetShip(targetShips[index]->shared_from_this());
		else
		{
			index -= targetShips.size();
			if(index < targetPlanets.size())
				ship.SetTargetStellar(targetPlanets[index]);
			else
				ship.SetTargetSystem(targetSystems[index - targetPlanets.size()]);
		}
	}
}



void AI::DoMining(Ship &ship, Command &command)
{
	// This function is only called for ships that are in the player's system.
	// Update the radius that the ship is searching for asteroids at.
	bool isNew = !miningAngle.contains(&ship);
	Angle &angle = miningAngle[&ship];
	if(isNew)
	{
		angle = Angle::Random();
		miningRadius[&ship] = ship.GetSystem()->AsteroidBeltRadius();
	}
	angle += Angle::Random(1.) - Angle::Random(1.);
	double radius = miningRadius[&ship] * pow(2., angle.Unit().X());

	shared_ptr<Minable> target = ship.GetTargetAsteroid();
	if(!target || target->Velocity().Length() > ship.MaxVelocity())
	{
		for(const shared_ptr<Minable> &minable : minables)
		{
			Point offset = minable->Position() - ship.Position();
			// Target only nearby minables that are within 45deg of the current heading
			// and not moving faster than the ship can catch.
			if(offset.Length() < 800. && offset.Unit().Dot(ship.Facing().Unit()) > .7
					&& minable->Velocity().Dot(offset.Unit()) < ship.MaxVelocity())
			{
				target = minable;
				ship.SetTargetAsteroid(target);
				break;
			}
		}
	}
	if(target)
	{
		// If the asteroid has moved well out of reach, stop tracking it.
		if(target->Position().Distance(ship.Position()) > 1600.)
			ship.SetTargetAsteroid(nullptr);
		else
		{
			MoveToAttack(ship, command, *target);
			AutoFire(ship, firingCommands, *target);
			return;
		}
	}

	Point heading = Angle(30.).Rotate(ship.Position().Unit() * radius) - ship.Position();
	command.SetTurn(TurnToward(ship, heading));
	if(ship.Velocity().Dot(heading.Unit()) < .7 * ship.MaxVelocity())
		command |= Command::FORWARD;
}



bool AI::DoHarvesting(Ship &ship, Command &command) const
{
	// If the ship has no target to pick up, do nothing.
	shared_ptr<Flotsam> target = ship.GetTargetFlotsam();
	// Don't try to chase flotsam that are already being pulled toward the ship by a tractor beam.
	const set<const Flotsam *> &avoid = ship.GetTractorFlotsam();
	if(target && (!ship.CanPickUp(*target) || avoid.contains(target.get())))
	{
		target.reset();
		ship.SetTargetFlotsam(target);
	}
	if(!target)
	{
		// Only check for new targets every 10 frames, on average.
		if(Random::Int(10))
			return false;

		// Don't chase anything that will take more than 10 seconds to reach.
		double bestTime = 600.;
		for(const shared_ptr<Flotsam> &it : flotsam)
		{
			if(!ship.CanPickUp(*it) || avoid.contains(it.get()))
				continue;
			// Only pick up flotsam that is nearby and that you are facing toward. Player escorts should
			// always attempt to pick up nearby flotsams when they are given a harvest order, and so ignore
			// the facing angle check.
			Point p = it->Position() - ship.Position();
			double range = p.Length();
			// Player ships do not have a restricted field of view so that they target flotsam behind them.
			if(range > 800. || (range > 100. && p.Unit().Dot(ship.Facing().Unit()) < .9 && !ship.IsYours()))
				continue;

			// Estimate how long it would take to intercept this flotsam.
			Point v = it->Velocity() - ship.Velocity();
			double vMax = ship.MaxVelocity();
			double time = RendezvousTime(p, v, vMax);
			if(std::isnan(time))
				continue;

			double degreesToTurn = TO_DEG * acos(min(1., max(-1., p.Unit().Dot(ship.Facing().Unit()))));
			time += degreesToTurn / ship.TurnRate();
			if(time < bestTime)
			{
				bestTime = time;
				target = it;
			}
		}
		if(!target)
			return false;

		ship.SetTargetFlotsam(target);
	}
	// Deploy any carried ships to improve maneuverability.
	if(ship.HasBays())
	{
		command |= Command::DEPLOY;
		Deploy(ship, false);
	}

	PickUp(ship, command, *target);
	return true;
}



// Check if this ship should cloak. Returns true if this ship decided to run away while cloaking.
bool AI::DoCloak(const Ship &ship, Command &command) const
{
	if(ship.GetPersonality().IsDecloaked())
		return false;
	double cloakingSpeed = ship.CloakingSpeed();
	if(!cloakingSpeed)
		return false;
	// Never cloak if it will cause you to be stranded.
	const Outfit &attributes = ship.Attributes();
	double cloakingFuel = attributes.Get("cloaking fuel");
	double fuelCost = cloakingFuel
		+ attributes.Get("fuel consumption") - attributes.Get("fuel generation");
	if(cloakingFuel && !attributes.Get("ramscoop"))
	{
		double fuel = ship.Fuel() * attributes.Get("fuel capacity");
		int steps = ceil((1. - ship.Cloaking()) / cloakingSpeed);
		// Only cloak if you will be able to fully cloak and also maintain it
		// for as long as it will take you to reach full cloak.
		fuel -= fuelCost * (1 + 2 * steps);
		if(fuel < ship.JumpNavigation().JumpFuel())
			return false;
	}

	// If your parent has chosen to cloak, cloak and rendezvous with them.
	const shared_ptr<const Ship> &parent = ship.GetParent();
	bool shouldCloakWithParent = false;
	if(parent && parent->GetGovernment() && parent->Commands().Has(Command::CLOAK)
			&& parent->GetSystem() == ship.GetSystem())
	{
		const Government *parentGovernment = parent->GetGovernment();
		bool isPlayer = parentGovernment->IsPlayer();
		if(isPlayer && ship.GetGovernment() == parentGovernment)
			shouldCloakWithParent = true;
		else if(isPlayer && ship.GetPersonality().IsEscort() && !ship.GetPersonality().IsUninterested())
			shouldCloakWithParent = true;
		else if(!isPlayer && !parent->GetGovernment()->IsEnemy(ship.GetGovernment()))
			shouldCloakWithParent = true;
	}
	if(shouldCloakWithParent)
	{
		command |= Command::CLOAK;
		KeepStation(ship, command, *parent);
		return true;
	}

	// Otherwise, always cloak if you are in imminent danger.
	static const double MAX_RANGE = 10000.;
	double range = MAX_RANGE;
	const Ship *nearestEnemy = nullptr;
	// Find the nearest targetable, in-system enemy that could attack this ship.
	const auto enemies = GetShipsList(ship, true);
	for(const auto &foe : enemies)
		if(!foe->IsDisabled())
		{
			double distance = ship.Position().Distance(foe->Position());
			if(distance < range)
			{
				range = distance;
				nearestEnemy = foe;
			}
		}

	// If this ship has started cloaking, it must get at least 40% repaired
	// or 40% farther away before it begins decloaking again.
	double hysteresis = ship.Commands().Has(Command::CLOAK) ? .4 : 0.;
	// If cloaking costs nothing, and no one has asked you for help, cloak at will.
	// Player ships should never cloak automatically if they are not in danger.
	bool cloakFreely = (fuelCost <= 0.) && !ship.GetShipToAssist() && !ship.IsYours();
	// If this ship is injured and can repair those injuries while cloaked,
	// then it should cloak while under threat.
	bool canRecoverShieldsCloaked = false;
	bool canRecoverHullCloaked = false;
	if(attributes.Get("cloaked regen multiplier") > -1.)
	{
		if(attributes.Get("shield generation") > 0.)
			canRecoverShieldsCloaked = true;
		else if(attributes.Get("cloaking shield delay") < 1. && attributes.Get("delayed shield generation") > 0.)
			canRecoverShieldsCloaked = true;
	}
	if(attributes.Get("cloaked repair multiplier") > -1.)
	{
		if(attributes.Get("hull repair rate") > 0.)
			canRecoverHullCloaked = true;
		else if(attributes.Get("cloaking repair delay") < 1. && attributes.Get("delayed hull repair") > 0.)
			canRecoverHullCloaked = true;
	}
	bool cloakToRepair = (ship.Health() < RETREAT_HEALTH + hysteresis)
			&& ((ship.Shields() < 1. && canRecoverShieldsCloaked)
			|| (ship.Hull() < 1. && canRecoverHullCloaked));
	if(cloakToRepair && (cloakFreely || range < 2000. * (1. + hysteresis)))
	{
		command |= Command::CLOAK;
		// Move away from the nearest enemy.
		if(nearestEnemy)
		{
			Point safety;
			// TODO: This could use an "Avoid" method, to account for other in-system hazards.
			// Simple approximation: move equally away from both the system center and the
			// nearest enemy, until the constrainment boundary is reached.
			if(ship.GetPersonality().IsUnconstrained() || !fenceCount.contains(&ship))
				safety = 2 * ship.Position().Unit() - nearestEnemy->Position().Unit();
			else
				safety = -ship.Position().Unit();

			safety *= ship.MaxVelocity();
			MoveTo(ship, command, ship.Position() + safety, safety, 1., .8);
			return true;
		}
	}
	// Choose to cloak if there are no enemies nearby and cloaking is sensible.
	if(range == MAX_RANGE && cloakFreely && !ship.GetTargetShip())
		command |= Command::CLOAK;

	return false;
}



void AI::DoPatrol(Ship &ship, Command &command) const
{
	double radius = ship.GetSystem()->ExtraHyperArrivalDistance();
	if(radius == 0.)
		radius = 500.;

	// The ship is outside of the effective range of the system,
	// so we turn it around.
	if(ship.Position().LengthSquared() > radius * radius)
	{
		// Allow ships to land after a while, otherwise they would continue to accumulate in the system.
		if(!ship.GetPersonality().IsStaying() && !Random::Int(10000))
		{
			vector<const StellarObject *> landingTargets;
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.HasSprite() && object.GetPlanet() && object.GetPlanet()->CanLand(ship))
					landingTargets.push_back(&object);
			if(!landingTargets.empty())
			{
				ship.SetTargetStellar(landingTargets[Random::Int(landingTargets.size())]);
				MoveToPlanet(ship, command);
				command |= Command::LAND;
				return;
			}
		}
		// Hacky way of differentiating ship behaviour without additional storage,
		// while keeping it consistent for each ship. TODO: change when Ship::SetTargetLocation exists.
		// This uses the pointer of the ship to choose a pseudo-random angle and instructs it to
		// patrol the system in a criss-crossing pattern, where each turn is this specific angle.
		intptr_t seed = reinterpret_cast<intptr_t>(&ship);
		int behaviour = abs(seed % 23);
		Angle delta = Angle(360. / (behaviour / 2. + 2.) * (behaviour % 2 ? -1. : 1.));
		Angle target = Angle(ship.Position()) + delta;
		MoveTo(ship, command, target.Unit() * radius / 2, Point(), 10., 1.);
	}
	// Otherwise, keep going forward.
	else
	{
		const Point targetVelocity = ship.Facing().Unit() * (ship.MaxVelocity() + 1);
		const Point targetPosition = ship.Position() + targetVelocity;
		MoveTo(ship, command, targetPosition, targetVelocity, 10., 1.);
	}
}



void AI::DoScatter(const Ship &ship, Command &command) const
{
	if(!command.Has(Command::FORWARD) && !command.Has(Command::BACK))
		return;

	double flip = command.Has(Command::BACK) ? -1 : 1;
	double turnRate = ship.TurnRate();
	double acceleration = ship.Acceleration();
	// TODO: If there are many ships, use CollisionSet::Circle or another
	// suitable method to limit which ships are checked.
	for(const shared_ptr<Ship> &other : ships)
	{
		// Do not scatter away from yourself, or ships in other systems.
		if(other.get() == &ship || other->GetSystem() != ship.GetSystem())
			continue;

		// Check for any ships that have nearly the same movement profile as
		// this ship and are in nearly the same location.
		Point offset = other->Position() - ship.Position();
		if(offset.LengthSquared() > 400.)
			continue;
		if(fabs(other->TurnRate() / turnRate - 1.) > .05)
			continue;
		if(fabs(other->Acceleration() / acceleration - 1.) > .05)
			continue;

		// We are too close to this ship. Turn away from it if we aren't already facing away.
		if(fabs(other->Facing().Unit().Dot(ship.Facing().Unit())) > 0.99) // 0.99 => 8 degrees
			command.SetTurn(flip * offset.Cross(ship.Facing().Unit()) > 0. ? 1. : -1.);
		return;
	}
}



bool AI::DoSecretive(Ship &ship, Command &command) const
{
	shared_ptr<Ship> scanningShip;
	// Figure out if any ship is currently scanning us. If that is the case, move away from it.
	for(auto &otherShip : GetShipsList(ship, false))
		if(!ship.GetGovernment()->Trusts(otherShip->GetGovernment()) &&
				otherShip->Commands().Has(Command::SCAN) &&
				otherShip->GetTargetShip() == ship.shared_from_this() &&
				!otherShip->IsDisabled() && !otherShip->IsDestroyed())
			scanningShip = make_shared<Ship>(*otherShip);

	if(scanningShip)
	{
		Point scanningPos = scanningShip->Position();
		Point pos = ship.Position();

		double cargoDistance = scanningShip->Attributes().Get("cargo scan power");
		double outfitDistance = scanningShip->Attributes().Get("outfit scan power");

		double maxScanRange = max(cargoDistance, outfitDistance);
		double distance = scanningPos.DistanceSquared(pos) * .0001;

		// If it can scan us we need to evade.
		if(distance < maxScanRange)
		{
			Point away;
			if(ship.GetPersonality().IsUnconstrained() || !fenceCount.contains(&ship))
				away = pos - scanningPos;
			else
				away = -pos;
			away *= ship.MaxVelocity();
			MoveTo(ship, command, pos + away, away, 1., 1.);
			return true;
		}
	}
	return false;
}



// Instead of coming to a full stop, adjust to a target velocity vector
Point AI::StoppingPoint(const Ship &ship, const Point &targetVelocity, bool &shouldReverse)
{
	Point position = ship.Position();
	Point velocity = ship.Velocity() - targetVelocity;
	Angle angle = ship.Facing();
	double acceleration = ship.CrewAcceleration();
	double turnRate = ship.CrewTurnRate();
	shouldReverse = false;

	// If I were to turn around and stop now the relative movement, where would that put me?
	double v = velocity.Length();
	if(!v)
		return position;
	// It makes no sense to calculate a stopping point for a ship entering hyperspace.
	if(ship.IsHyperspacing())
	{
		if(ship.IsUsingJumpDrive() || ship.IsEnteringHyperspace())
			return position;

		double maxVelocity = ship.MaxVelocity();
		double jumpTime = (v - maxVelocity) / 2.;
		position += velocity.Unit() * (jumpTime * (v + maxVelocity) * .5);
		v = maxVelocity;
	}

	// This assumes you're facing exactly the wrong way.
	double degreesToTurn = TO_DEG * acos(min(1., max(-1., -velocity.Unit().Dot(angle.Unit()))));
	double stopDistance = v * (degreesToTurn / turnRate);
	// Sum of: v + (v - a) + (v - 2a) + ... + 0.
	// The number of terms will be v / a.
	// The average term's value will be v / 2. So:
	stopDistance += .5 * v * v / acceleration;

	if(ship.Attributes().Get("reverse thrust"))
	{
		// Figure out your reverse thruster stopping distance:
		double reverseAcceleration = ship.Attributes().Get("reverse thrust") / ship.InertialMass();
		double reverseDistance = v * (180. - degreesToTurn) / turnRate;
		reverseDistance += .5 * v * v / reverseAcceleration;

		if(reverseDistance < stopDistance)
		{
			shouldReverse = true;
			stopDistance = reverseDistance;
		}
	}

	return position + stopDistance * velocity.Unit();
}



// Get a vector giving the direction this ship should aim in in order to do
// maximum damaged to a target at the given position with its non-turret,
// non-homing weapons. If the ship has no non-homing weapons, this just
// returns the direction to the target.
Point AI::TargetAim(const Ship &ship)
{
	shared_ptr<const Ship> target = ship.GetTargetShip();
	if(target)
		return TargetAim(ship, *target);

	shared_ptr<const Minable> targetAsteroid = ship.GetTargetAsteroid();
	if(targetAsteroid)
		return TargetAim(ship, *targetAsteroid);

	return Point();
}



Point AI::TargetAim(const Ship &ship, const Body &target)
{
	Point result;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetWeapon();
		if(!weapon || hardpoint.IsHoming() || hardpoint.IsTurret())
			continue;

		Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
		Point p = target.Position() - start + ship.GetPersonality().Confusion();
		Point v = target.Velocity() - ship.Velocity();
		double steps = RendezvousTime(p, v, weapon->WeightedVelocity() + .5 * weapon->RandomVelocity());
		if(std::isnan(steps))
			continue;

		steps = min(steps, weapon->TotalLifetime());
		p += steps * v;

		double damage = weapon->ShieldDamage() + weapon->HullDamage();
		result += p.Unit() * abs(damage);
	}

	return result ? result : target.Position() - ship.Position();
}



// Aim the given ship's turrets.
void AI::AimTurrets(const Ship &ship, FireCommand &command, bool opportunistic,
		const optional<Point> &targetOverride) const
{
	// (Position, Velocity) pairs of the targets.
	vector<pair<Point, Point>> targets;
	if(!targetOverride)
	{
		// First, get the set of potential hostile ships.
		vector<const Body *> targetBodies;
		const Ship *currentTarget = ship.GetTargetShip().get();
		if(opportunistic || !currentTarget || !currentTarget->IsTargetable())
		{
			// Find the maximum range of any of this ship's turrets.
			double maxRange = 0.;
			for(const Hardpoint &hardpoint : ship.Weapons())
				if(hardpoint.CanAim(ship))
					maxRange = max(maxRange, hardpoint.GetWeapon()->Range());
			// If this ship has no turrets, bail out.
			if(!maxRange)
				return;
			// Extend the weapon range slightly to account for velocity differences.
			maxRange *= 1.5;

			// Now, find all enemy ships within that radius.
			auto enemies = GetShipsList(ship, true, maxRange);
			// Convert the shared_ptr<Ship> into const Body *, to allow aiming turrets
			// at a targeted asteroid. Skip disabled ships, which pose no threat.
			for(auto &&foe : enemies)
				if(!foe->IsDisabled())
					targetBodies.emplace_back(foe);
			// Even if the ship's current target ship is beyond maxRange,
			// or is already disabled, consider aiming at it.
			if(currentTarget && currentTarget->IsTargetable()
					&& find(targetBodies.cbegin(), targetBodies.cend(), currentTarget) == targetBodies.cend())
				targetBodies.push_back(currentTarget);
		}
		else
			targetBodies.push_back(currentTarget);
		// If this ship is mining, consider aiming at its target asteroid.
		if(ship.GetTargetAsteroid())
			targetBodies.push_back(ship.GetTargetAsteroid().get());

		// If there are no targets to aim at, opportunistic turrets should sweep
		// back and forth at random, with the sweep centered on the "outward-facing"
		// angle. Focused turrets should just point forward.
		if(targetBodies.empty() && !opportunistic)
		{
			for(const Hardpoint &hardpoint : ship.Weapons())
				if(hardpoint.CanAim(ship))
				{
					// Get the index of this weapon.
					int index = &hardpoint - &ship.Weapons().front();
					double offset = (hardpoint.GetIdleAngle() - hardpoint.GetAngle()).Degrees();
					command.SetAim(index, offset / hardpoint.TurnRate(ship));
				}
			return;
		}
		if(targetBodies.empty())
		{
			for(const Hardpoint &hardpoint : ship.Weapons())
				if(hardpoint.CanAim(ship))
				{
					// Get the index of this weapon.
					int index = &hardpoint - &ship.Weapons().front();
					// First, check if this turret is currently in motion. If not,
					// it only has a small chance of beginning to move.
					double previous = ship.FiringCommands().Aim(index);
					if(!previous && Random::Int(60))
						continue;

					// Sweep between the min and max arc.
					Angle centerAngle = Angle(hardpoint.GetIdleAngle());
					const Angle minArc = hardpoint.GetMinArc();
					const Angle maxArc = hardpoint.GetMaxArc();
					const double arcMiddleDegrees = (minArc.AbsDegrees() + maxArc.AbsDegrees()) / 2.;
					double bias = (centerAngle - hardpoint.GetAngle()).Degrees() / min(arcMiddleDegrees, 180.);
					double acceleration = Random::Real() - Random::Real() + bias;
					command.SetAim(index, previous + .1 * acceleration);
				}
			return;
		}

		targets.reserve(targetBodies.size());
		for(auto body : targetBodies)
			targets.emplace_back(body->Position(), body->Velocity());
	}
	else
		targets.emplace_back(*targetOverride + ship.Position(), ship.Velocity());
	// Each hardpoint should aim at the target that it is "closest" to hitting.
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.CanAim(ship))
		{
			// This is where this projectile fires from. Add some randomness
			// based on how skilled the pilot is.
			Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
			start += ship.GetPersonality().Confusion();
			// Get the turret's current facing, in absolute coordinates:
			Angle aim = ship.Facing() + hardpoint.GetAngle();
			// Get this projectile's average velocity.
			const Weapon *weapon = hardpoint.GetWeapon();
			double vp = weapon->WeightedVelocity() + .5 * weapon->RandomVelocity();
			// Loop through each body this hardpoint could shoot at. Find the
			// one that is the "best" in terms of how many frames it will take
			// to aim at it and for a projectile to hit it.
			double bestScore = numeric_limits<double>::infinity();
			double bestAngle = 0.;
			for(auto [p, v] : targets)
			{
				p -= start;

				// Only take the ship's velocity into account if this weapon
				// does not have its own acceleration.
				if(!weapon->Acceleration())
					v -= ship.Velocity();
				// By the time this action is performed, the target will
				// have moved forward one time step.
				p += v;

				double rendezvousTime = numeric_limits<double>::quiet_NaN();
				double distance = p.Length();
				// Beam weapons hit instantaneously if they are in range.
				bool isInstantaneous = weapon->TotalLifetime() == 1.;
				if(isInstantaneous && distance < vp)
					rendezvousTime = 0.;
				else
				{
					// Find out how long it would take for this projectile to reach the target.
					if(!isInstantaneous)
						rendezvousTime = RendezvousTime(p, v, vp);

					// If there is no intersection (i.e. the turret is not facing the target),
					// consider this target "out-of-range" but still targetable.
					if(std::isnan(rendezvousTime))
						rendezvousTime = max(distance / (vp ? vp : 1.), 2 * weapon->TotalLifetime());

					// Determine where the target will be at that point.
					p += v * rendezvousTime;

					// All bodies within weapons range have the same basic
					// weight. Outside that range, give them lower priority.
					rendezvousTime = max(0., rendezvousTime - weapon->TotalLifetime());
				}

				// Determine how much the turret must turn to face that vector.
				double degrees = 0.;
				Angle angleToPoint = Angle(p);
				if(hardpoint.IsOmnidirectional())
					degrees = (angleToPoint - aim).Degrees();
				else
				{
					// For turret with limited arc, determine the turn up to the nearest arc limit.
					// Also reduce priority of target if it's not within the firing arc.
					const Angle facing = ship.Facing();
					const Angle minArc = hardpoint.GetMinArc() + facing;
					const Angle maxArc = hardpoint.GetMaxArc() + facing;
					if(!angleToPoint.IsInRange(minArc, maxArc))
					{
						// Decrease the priority of the target.
						rendezvousTime += 2. * weapon->TotalLifetime();

						// Point to the nearer edge of the arc.
						const double minDegree = (minArc - angleToPoint).Degrees();
						const double maxDegree = (maxArc - angleToPoint).Degrees();
						if(fabs(minDegree) < fabs(maxDegree))
							angleToPoint = minArc;
						else
							angleToPoint = maxArc;
					}
					degrees = (angleToPoint - minArc).AbsDegrees() - (aim - minArc).AbsDegrees();
				}
				double turnTime = fabs(degrees) / hardpoint.TurnRate(ship);
				// Always prefer targets that you are able to hit.
				double score = turnTime + (180. / hardpoint.TurnRate(ship)) * rendezvousTime;
				if(score < bestScore)
				{
					bestScore = score;
					bestAngle = degrees;
				}
			}
			if(bestAngle)
			{
				// Get the index of this weapon.
				int index = &hardpoint - &ship.Weapons().front();
				command.SetAim(index, bestAngle / hardpoint.TurnRate(ship));
			}
		}
}



// Fire whichever of the given ship's weapons can hit a hostile target.
void AI::AutoFire(const Ship &ship, FireCommand &command, bool secondary, bool isFlagship) const
{
	const Personality &person = ship.GetPersonality();
	if(person.IsPacifist() || ship.CannotAct(Ship::ActionType::FIRE))
		return;

	bool beFrugal = (ship.IsYours() && !escortsUseAmmo);
	if(person.IsFrugal() || (ship.IsYours() && escortsAreFrugal && escortsUseAmmo))
	{
		// The frugal personality is only active when ships have more than a certain fraction of their total health,
		// and are not outgunned. The default threshold is 75%.
		beFrugal = (ship.Health() > GameData::GetGamerules().UniversalFrugalThreshold());
		if(beFrugal)
		{
			auto ait = allyStrength.find(ship.GetGovernment());
			auto eit = enemyStrength.find(ship.GetGovernment());
			if(ait != allyStrength.end() && eit != enemyStrength.end() && ait->second < eit->second)
				beFrugal = false;
		}
	}

	// Special case: your target is not your enemy. Do not fire, because you do
	// not want to risk damaging that target. Ships will target friendly ships
	// while assisting and performing surveillance.
	shared_ptr<Ship> currentTarget = ship.GetTargetShip();
	const Government *gov = ship.GetGovernment();
	bool friendlyOverride = false;
	bool disabledOverride = false;
	if(ship.IsYours())
	{
		auto it = orders.find(&ship);
		if(it != orders.end())
		{
			if(it->second.Has(Orders::Types::HOLD_FIRE))
				return;
			if(it->second.GetTargetShip() == currentTarget)
			{
				disabledOverride = it->second.Has(Orders::Types::FINISH_OFF);
				friendlyOverride = disabledOverride || it->second.Has(Orders::Types::ATTACK);
			}
		}
	}
	bool currentIsEnemy = currentTarget
		&& currentTarget->GetGovernment()->IsEnemy(gov)
		&& currentTarget->GetSystem() == ship.GetSystem();
	if(currentTarget && !(currentIsEnemy || friendlyOverride))
		currentTarget.reset();

	// Only fire on disabled targets if you don't want to plunder them.
	bool plunders = (person.Plunders() && ship.Cargo().Free());
	bool disables = person.Disables();

	// Don't use weapons with firing force if you are preparing to jump.
	bool isWaitingToJump = ship.Commands().Has(Command::JUMP | Command::WAIT);

	// Find the longest range of any of your non-homing weapons. Homing weapons
	// that don't consume ammo may also fire in non-homing mode.
	double maxRange = 0.;
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.IsReady())
		{
			const Weapon *weapon = hardpoint.GetWeapon();
			if(!(!currentTarget && hardpoint.IsHoming() && weapon->Ammo())
					&& !(!secondary && weapon->Icon())
					&& !(beFrugal && weapon->Ammo())
					&& !(isWaitingToJump && weapon->FiringForce()))
				maxRange = max(maxRange, weapon->Range());
		}
	// Extend the weapon range slightly to account for velocity differences.
	maxRange *= 1.5;

	// Find all enemy ships within range of at least one weapon.
	auto enemies = GetShipsList(ship, true, maxRange);
	// Consider the current target if it is not already considered (i.e. it
	// is a friendly ship and this is a player ship ordered to attack it).
	if(currentTarget && currentTarget->IsTargetable()
			&& find(enemies.cbegin(), enemies.cend(), currentTarget.get()) == enemies.cend())
		enemies.push_back(currentTarget.get());

	int index = -1;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		++index;
		// Skip weapons that are not ready to fire.
		if(!hardpoint.IsReady())
			continue;

		// Skip weapons omitted by the "Automatic firing" preference.
		if(isFlagship)
		{
			const Preferences::AutoFire autoFireMode = Preferences::GetAutoFire();
			if(autoFireMode == Preferences::AutoFire::GUNS_ONLY && hardpoint.IsTurret())
				continue;
			if(autoFireMode == Preferences::AutoFire::TURRETS_ONLY && !hardpoint.IsTurret())
				continue;
		}

		const Weapon *weapon = hardpoint.GetWeapon();
		// Don't expend ammo for homing weapons that have no target selected.
		if(!currentTarget && weapon->Homing() && weapon->Ammo())
			continue;
		// Don't fire secondary weapons if told not to.
		if(!secondary && weapon->Icon())
			continue;
		// Don't expend ammo if trying to be frugal.
		if(beFrugal && weapon->Ammo())
			continue;
		// Don't use weapons with firing force if you are preparing to jump.
		if(isWaitingToJump && weapon->FiringForce())
			continue;

		// Special case: if the weapon uses fuel, be careful not to spend so much
		// fuel that you cannot leave the system if necessary.
		if(weapon->FiringFuel())
		{
			double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
			fuel -= weapon->FiringFuel();
			// If the ship is not ever leaving this system, it does not need to
			// reserve any fuel.
			bool isStaying = person.IsStaying();
			if(!secondary || fuel < (isStaying ? 0. : ship.JumpNavigation().JumpFuel()))
				continue;
		}
		// Figure out where this weapon will fire from, but add some randomness
		// depending on how accurate this ship's pilot is.
		Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
		start += person.Confusion();

		double vp = weapon->WeightedVelocity() + .5 * weapon->RandomVelocity();
		double lifetime = weapon->TotalLifetime();

		// Homing weapons revert to "dumb firing" if they have no target.
		if(weapon->Homing() && currentTarget)
		{
			// NPCs shoot ships that they just plundered.
			bool hasBoarded = !ship.IsYours() && Has(ship, currentTarget, ShipEvent::BOARD);
			if(currentTarget->IsDisabled() && (disables || (plunders && !hasBoarded)) && !disabledOverride)
				continue;
			// Don't fire secondary weapons at targets that have started jumping.
			if(weapon->Icon() && currentTarget->IsEnteringHyperspace())
				continue;

			// For homing weapons, don't take the velocity of the ship firing it
			// into account, because the projectile will settle into a velocity
			// that depends on its own acceleration and drag.
			Point p = currentTarget->Position() - start;
			Point v = currentTarget->Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;

			// If this weapon has a blast radius, don't fire it if the target is
			// so close that you'll be hit by the blast. Weapons using proximity
			// triggers will explode sooner, so a larger separation is needed.
			if(!weapon->IsSafe() && p.Length() <= (weapon->BlastRadius() + weapon->TriggerRadius()))
				continue;

			// Calculate how long it will take the projectile to reach its target.
			double steps = RendezvousTime(p, v, vp);
			if(!std::isnan(steps) && steps <= lifetime)
			{
				command.SetFire(index);
				continue;
			}
			continue;
		}
		// For non-homing weapons:
		for(const auto &target : enemies)
		{
			// NPCs shoot ships that they just plundered.
			bool hasBoarded = !ship.IsYours() && Has(ship, target->shared_from_this(), ShipEvent::BOARD);
			if(target->IsDisabled() && (disables || (plunders && !hasBoarded)) && !disabledOverride)
				continue;
			// Merciful ships let fleeing ships go.
			if(target->IsFleeing() && person.IsMerciful())
				continue;
			// Don't hit ships that cannot be hit without targeting
			if(target != currentTarget.get() && !FighterHitHelper::IsValidTarget(target))
				continue;

			Point p = target->Position() - start;
			Point v = target->Velocity();
			// Only take the ship's velocity into account if this weapon
			// does not have its own acceleration.
			if(!weapon->Acceleration())
				v -= ship.Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;

			// Non-homing weapons may have a blast radius or proximity trigger.
			// Do not fire this weapon if we will be caught in the blast.
			if(!weapon->IsSafe() && p.Length() <= (weapon->BlastRadius() + weapon->TriggerRadius()))
				continue;

			// Get the vector the weapon will travel along.
			v = (ship.Facing() + hardpoint.GetAngle()).Unit() * vp - v;
			// Extrapolate over the lifetime of the projectile.
			v *= lifetime;

			const Mask &mask = target->GetMask(step);
			if(mask.Collide(-p, v, target->Facing()) < 1.)
			{
				command.SetFire(index);
				break;
			}
		}
	}
}



void AI::AutoFire(const Ship &ship, FireCommand &command, const Body &target) const
{
	int index = -1;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		++index;
		// Only auto-fire primary weapons that take no ammunition.
		if(!hardpoint.IsReady())
			continue;
		const Weapon *weapon = hardpoint.GetWeapon();
		if(weapon->Icon() || weapon->Ammo())
			continue;

		// Figure out where this weapon will fire from, but add some randomness
		// depending on how accurate this ship's pilot is.
		Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
		start += ship.GetPersonality().Confusion();

		double vp = weapon->WeightedVelocity() + .5 * weapon->RandomVelocity();
		double lifetime = weapon->TotalLifetime();

		Point p = target.Position() - start;
		Point v = target.Velocity();
		// Only take the ship's velocity into account if this weapon
		// does not have its own acceleration.
		if(!weapon->Acceleration())
			v -= ship.Velocity();
		// By the time this action is performed, the ships will have moved
		// forward one time step.
		p += v;

		// Get the vector the weapon will travel along.
		v = (ship.Facing() + hardpoint.GetAngle()).Unit() * vp - v;
		// Extrapolate over the lifetime of the projectile.
		v *= lifetime;

		const Mask &mask = target.GetMask(step);
		if(mask.Collide(-p, v, target.Facing()) < 1.)
			command.SetFire(index);
	}
}



// Get the amount of time it would take the given weapon to reach the given
// target, assuming it can be fired in any direction (i.e. turreted). For
// non-turreted weapons this can be used to calculate the ideal direction to
// point the ship in.
double AI::RendezvousTime(const Point &p, const Point &v, double vp)
{
	// How many steps will it take this projectile
	// to intersect the target?
	// (p.x + v.x*t)^2 + (p.y + v.y*t)^2 = vp^2*t^2
	// p.x^2 + 2*p.x*v.x*t + v.x^2*t^2
	//    + p.y^2 + 2*p.y*v.y*t + v.y^2t^2
	//    - vp^2*t^2 = 0
	// (v.x^2 + v.y^2 - vp^2) * t^2
	//    + (2 * (p.x * v.x + p.y * v.y)) * t
	//    + (p.x^2 + p.y^2) = 0
	double a = v.Dot(v) - vp * vp;
	double b = 2. * p.Dot(v);
	double c = p.Dot(p);
	double discriminant = b * b - 4 * a * c;
	if(discriminant < 0.)
		return numeric_limits<double>::quiet_NaN();

	discriminant = sqrt(discriminant);

	// The solutions are b +- discriminant.
	// But it's not a solution if it's negative.
	double r1 = (-b + discriminant) / (2. * a);
	double r2 = (-b - discriminant) / (2. * a);
	if(r1 >= 0. && r2 >= 0.)
		return min(r1, r2);
	else if(r1 >= 0. || r2 >= 0.)
		return max(r1, r2);

	return numeric_limits<double>::quiet_NaN();
}



// Searches every asteroid within the ship scan limit and returns either the
// asteroid closest to the ship or the asteroid of highest value in range, depending
// on the player's preferences.
bool AI::TargetMinable(Ship &ship) const
{
	double scanRangeMetric = 10000. * ship.Attributes().Get("asteroid scan power");
	if(!scanRangeMetric)
		return false;
	const bool findClosest = Preferences::Has("Target asteroid based on");
	auto bestMinable = ship.GetTargetAsteroid();
	double bestScore = findClosest ? numeric_limits<double>::max() : 0.;
	auto GetDistanceMetric = [&ship](const Minable &minable) -> double {
		return ship.Position().DistanceSquared(minable.Position());
	};
	if(bestMinable)
	{
		if(findClosest)
			bestScore = GetDistanceMetric(*bestMinable);
		else
			bestScore = bestMinable->GetValue();
	}
	auto MinableStrategy = [&findClosest, &bestMinable, &bestScore, &GetDistanceMetric]()
			-> function<void(const shared_ptr<Minable> &)>
	{
		if(findClosest)
			return [&bestMinable, &bestScore, &GetDistanceMetric]
					(const shared_ptr<Minable> &minable) -> void {
				double newScore = GetDistanceMetric(*minable);
				if(newScore < bestScore || (newScore == bestScore && minable->GetValue() > bestMinable->GetValue()))
				{
					bestScore = newScore;
					bestMinable = minable;
				}
			};
		else
			return [&bestMinable, &bestScore, &GetDistanceMetric]
					(const shared_ptr<Minable> &minable) -> void {
				double newScore = minable->GetValue();
				if(newScore > bestScore || (newScore == bestScore
						&& GetDistanceMetric(*minable) < GetDistanceMetric(*bestMinable)))
				{
					bestScore = newScore;
					bestMinable = minable;
				}
			};
	};
	auto UpdateBestMinable = MinableStrategy();
	for(auto &&minable : minables)
	{
		if(GetDistanceMetric(*minable) > scanRangeMetric)
			continue;
		if(bestMinable)
			UpdateBestMinable(minable);
		else
			bestMinable = minable;
	}
	if(bestMinable)
		ship.SetTargetAsteroid(bestMinable);
	return static_cast<bool>(ship.GetTargetAsteroid());
}



void AI::MovePlayer(Ship &ship, Command &activeCommands)
{
	Command command;
	firingCommands.SetHardpoints(ship.Weapons().size());

	bool shift = activeCommands.Has(Command::SHIFT);

	bool isWormhole = false;
	if(player.HasTravelPlan())
	{
		// Determine if the player is jumping to their target system or landing on a wormhole.
		const System *system = player.TravelPlan().back();
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsWormhole()
				&& object.GetPlanet()->IsAccessible(&ship) && player.HasVisited(*object.GetPlanet())
				&& player.CanView(*system))
			{
				const auto *wormhole = object.GetPlanet()->GetWormhole();
				if(&wormhole->WormholeDestination(*ship.GetSystem()) != system)
					continue;

				isWormhole = true;
				if(!ship.GetTargetStellar() || autoPilot.Has(Command::JUMP))
					ship.SetTargetStellar(&object);
				break;
			}
		if(!isWormhole)
			ship.SetTargetSystem(system);
	}

	if(ship.IsEnteringHyperspace() && !ship.IsHyperspacing())
	{
		// Check if there's a particular planet there we want to visit.
		const System *system = ship.GetTargetSystem();
		set<const Planet *> destinations;
		Date deadline;
		const Planet *bestDestination = nullptr;
		size_t missions = 0;
		for(const Mission &mission : player.Missions())
		{
			// Don't include invisible and failed missions in the check.
			if(!mission.IsVisible() || mission.IsFailed())
				continue;

			// If the accessible destination of a mission is in this system, and you've been
			// to all waypoints and stopovers (i.e. could complete it), consider landing on it.
			if(mission.Stopovers().empty() && mission.Waypoints().empty()
					&& mission.Destination()->IsInSystem(system)
					&& mission.Destination()->IsAccessible(&ship))
			{
				destinations.insert(mission.Destination());
				++missions;
				// If this mission has a deadline, check if it is the soonest
				// deadline. If so, this should be your ship's destination.
				if(!deadline || (mission.Deadline() && mission.Deadline() < deadline))
				{
					deadline = mission.Deadline();
					bestDestination = mission.Destination();
				}
			}
			// Also check for stopovers in the destination system.
			for(const Planet *planet : mission.Stopovers())
				if(planet->IsInSystem(system) && planet->IsAccessible(&ship))
				{
					destinations.insert(planet);
					++missions;
					if(!bestDestination)
						bestDestination = planet;
				}
		}

		// Inform the player of any destinations in the system they are jumping to.
		if(!destinations.empty() && Preferences::GetNotificationSetting() != Preferences::NotificationSetting::OFF)
		{
			string message = "Note: you have ";
			message += (missions == 1 ? "a mission that requires" : "missions that require");
			message += " landing on ";
			message += Format::List<set, const Planet *>(destinations,
				[](const Planet *const &planet)
				{
					return planet->DisplayName();
				});
			message += " in the system you are jumping to.";
			Messages::Add({message, GameData::MessageCategories().Get("info")});

			if(Preferences::GetNotificationSetting() == Preferences::NotificationSetting::BOTH)
				UI::PlaySound(UI::UISound::FAILURE);
		}
		// If any destination was found, find the corresponding stellar object
		// and set it as your ship's target planet.
		if(bestDestination)
			ship.SetTargetStellar(system->FindStellar(bestDestination));
	}

	if(activeCommands.Has(Command::NEAREST))
	{
		// Find the nearest enemy ship to the flagship. If `Shift` is held, consider friendly ships too.
		double closest = numeric_limits<double>::infinity();
		bool foundActive = false;
		bool found = false;
		for(const shared_ptr<Ship> &other : ships)
			if(other.get() != &ship && other->IsTargetable())
			{
				bool enemy = other->GetGovernment()->IsEnemy(ship.GetGovernment());
				// Do not let "target nearest" select a friendly ship, so that
				// if the player is repeatedly targeting nearest to, say, target
				// a bunch of fighters, they won't start firing on friendly
				// ships as soon as the last one is gone.
				if((!enemy && !shift) || other->IsYours())
					continue;

				// Sort ships by active or disabled:
				// Prefer targeting an active ship over a disabled one
				bool active = !other->IsDisabled();

				double d = other->Position().Distance(ship.Position());

				if((!foundActive && active) || (foundActive == active && d < closest))
				{
					ship.SetTargetShip(other);
					closest = d;
					foundActive = active;
					found = true;
				}
			}
		// If no ship was found, look for nearby asteroids.
		if(!found)
			TargetMinable(ship);
		else
			UI::PlaySound(UI::UISound::TARGET);
	}
	else if(activeCommands.Has(Command::TARGET))
	{
		// Find the "next" ship to target. Holding `Shift` will cycle through escorts.
		shared_ptr<const Ship> target = ship.GetTargetShip();
		// Whether the next eligible ship should be targeted.
		bool selectNext = !target || !target->IsTargetable();
		for(const shared_ptr<Ship> &other : ships)
		{
			// Do not target yourself.
			if(other.get() == &ship)
				continue;
			// The default behavior is to ignore your fleet and any friendly escorts.
			bool isPlayer = other->IsYours() || (other->GetPersonality().IsEscort()
					&& !other->GetGovernment()->IsEnemy());
			if(other == target)
				selectNext = true;
			else if(selectNext && isPlayer == shift && other->IsTargetable())
			{
				ship.SetTargetShip(other);
				if(isPlayer)
					player.SelectEscort(other.get(), false);
				selectNext = false;
				break;
			}
		}
		if(selectNext)
		{
			ship.SetTargetShip(shared_ptr<Ship>());
			player.SelectEscort(nullptr, false);
		}
		else
			UI::PlaySound(UI::UISound::TARGET);
	}
	else if(activeCommands.Has(Command::BOARD))
	{
		// Determine the player's boarding target based on their current target and their boarding preference. They may
		// press BOARD repeatedly to cycle between ships, or use SHIFT to prioritize repairing their owned escorts.
		shared_ptr<Ship> target = ship.GetTargetShip();
		if(target && !CanBoard(ship, *target))
			target.reset();
		if(!target || activeCommands.Has(Command::WAIT) || (shift && !target->IsYours()))
		{
			if(shift)
				ship.SetTargetShip(shared_ptr<Ship>());

			const auto boardingPriority = Preferences::GetBoardingPriority();
			auto strategy = [&]() noexcept -> function<double(const Ship &)>
			{
				Point current = ship.Position();
				switch(boardingPriority)
				{
					case Preferences::BoardingPriority::VALUE:
						return [this, &ship](const Ship &other) noexcept -> double
						{
							// Use the exact cost if the ship was scanned, otherwise use an estimation.
							return this->Has(ship, other.shared_from_this(), ShipEvent::SCAN_OUTFITS) ?
								other.Cost() : (other.ChassisCost() * 2.);
						};
					case Preferences::BoardingPriority::MIXED:
						return [this, &ship, current](const Ship &other) noexcept -> double
						{
							double cost = this->Has(ship, other.shared_from_this(), ShipEvent::SCAN_OUTFITS) ?
								other.Cost() : (other.ChassisCost() * 2.);
							// Even if we divide by 0, doubles can contain and handle infinity,
							// and we should definitely board that one then.
							return cost * cost / (current.DistanceSquared(other.Position()) + 0.1);
						};
					case Preferences::BoardingPriority::PROXIMITY:
					default:
						return [current](const Ship &other) noexcept -> double
						{
							return current.DistanceSquared(other.Position());
						};
				}
			}();

			using ShipValue = pair<Ship *, double>;
			auto options = vector<ShipValue>{};
			if(shift)
			{
				const auto &owned = governmentRosters[ship.GetGovernment()];
				options.reserve(owned.size());
				for(auto &&escort : owned)
					if(CanBoard(ship, *escort))
						options.emplace_back(escort, strategy(*escort));
			}
			else
			{
				auto ships = GetShipsList(ship, true);
				options.reserve(ships.size());
				// The current target is not considered by GetShipsList.
				if(target)
					options.emplace_back(target.get(), strategy(*target));

				// First check if we can board enemy ships, then allies.
				for(auto &&enemy : ships)
					if(CanBoard(ship, *enemy))
						options.emplace_back(enemy, strategy(*enemy));
				if(options.empty())
				{
					ships = GetShipsList(ship, false);
					options.reserve(ships.size());
					for(auto &&ally : ships)
						if(CanBoard(ship, *ally))
							options.emplace_back(ally, strategy(*ally));
				}
			}

			if(options.empty())
				activeCommands.Clear(Command::BOARD);
			else
			{
				// Sort the list of options in increasing order of desirability.
				sort(options.begin(), options.end(),
					[&ship, boardingPriority](const ShipValue &lhs, const ShipValue &rhs)
					{
						if(boardingPriority == Preferences::BoardingPriority::PROXIMITY)
							return lhs.second > rhs.second;

						// If their cost is the same, prefer the closest ship.
						return (boardingPriority == Preferences::BoardingPriority::VALUE && lhs.second == rhs.second)
							? lhs.first->Position().DistanceSquared(ship.Position()) >
								rhs.first->Position().DistanceSquared(ship.Position())
							: lhs.second < rhs.second;
					}
				);

				// Pick the (next) most desirable option.
				auto it = !target ? options.end() : find_if(options.begin(), options.end(),
					[&target](const ShipValue &lhs) noexcept -> bool { return lhs.first == target.get(); });
				if(it == options.begin())
					it = options.end();
				ship.SetTargetShip((--it)->first->shared_from_this());
				UI::PlaySound(UI::UISound::TARGET);
			}
		}
	}
	// Player cannot attempt to land while departing from a planet.
	else if(activeCommands.Has(Command::LAND) && !ship.IsEnteringHyperspace() && ship.Zoom() == 1.)
	{
		// Track all possible landable objects in the current system.
		auto landables = vector<const StellarObject *>{};

		string message;
		const bool isMovingSlowly = (ship.Velocity().Length() < (MIN_LANDING_VELOCITY / 60.));
		const StellarObject *potentialTarget = nullptr;
		for(const StellarObject &object : ship.GetSystem()->Objects())
		{
			if(!object.HasSprite())
				continue;

			// If the player is moving slowly over an object, then the player is considering landing there.
			// The target object might not be able to be landed on, for example an enemy planet or a star.
			const bool isTryingLanding = (ship.Position().Distance(object.Position()) < object.Radius() && isMovingSlowly);
			if(object.HasValidPlanet() && object.GetPlanet()->IsAccessible(&ship) && object.IsVisible(ship.Position()))
			{
				landables.emplace_back(&object);
				if(isTryingLanding)
					potentialTarget = &object;
			}
			else if(isTryingLanding)
				message = object.LandingMessage();
		}

		const StellarObject *target = ship.GetTargetStellar();
		// Require that the player's planetary target is one of the current system's planets.
		auto landIt = find(landables.cbegin(), landables.cend(), target);
		if(landIt == landables.cend())
			target = nullptr;

		// Consider the potential target as a landing target first.
		if(!target && potentialTarget)
		{
			target = potentialTarget;
			ship.SetTargetStellar(potentialTarget);
		}

		// If the player has a target in mind already, don't emit an error if the player
		// is hovering above a star or inaccessible planet.
		if(target)
			message.clear();
		else if(!message.empty())
			UI::PlaySound(UI::UISound::FAILURE);

		const Message::Category *messageCategory = GameData::MessageCategories().Get("normal");

		if(target && (ship.Zoom() < 1. || ship.Position().Distance(target->Position()) < target->Radius()))
		{
			// Special case: if there are two planets in system and you have one
			// selected, then press "land" again, do not toggle to the other if
			// you are within landing range of the one you have selected.
		}
		else if(message.empty() && target && activeCommands.Has(Command::WAIT))
		{
			// Select the next landable in the list after the currently selected object.
			if(++landIt == landables.cend())
				landIt = landables.cbegin();
			const StellarObject *next = *landIt;
			ship.SetTargetStellar(next);

			if(!next->GetPlanet()->CanLand())
			{
				message = "The authorities on this " + next->GetPlanet()->Noun() +
					" refuse to clear you to land here.";
				messageCategory = GameData::MessageCategories().Get("high");
				UI::PlaySound(UI::UISound::FAILURE);
			}
			else if(next != target)
				message = "Switching landing targets. Now landing on " + next->DisplayName() + ".";
		}
		else if(message.empty())
		{
			// This is the first press, or it has been long enough since the last press,
			// so land on the nearest eligible planet. Prefer inhabited ones with fuel.
			set<string> types;
			if(!target && !landables.empty())
			{
				if(landables.size() == 1)
					ship.SetTargetStellar(landables.front());
				else
				{
					double closest = numeric_limits<double>::infinity();
					for(const auto &object : landables)
					{
						double distance = ship.Position().Distance(object->Position());
						const Planet *planet = object->GetPlanet();
						types.insert(planet->Noun());
						if((!planet->CanLand()
								|| !planet->GetPort().CanRecharge(Port::RechargeType::Fuel, ship.IsYours()))
								&& !planet->IsWormhole())
							distance += 10000.;

						if(distance < closest)
						{
							ship.SetTargetStellar(object);
							closest = distance;
						}
					}
				}
				target = ship.GetTargetStellar();
			}

			if(!target)
			{
				Messages::Add(*GameData::Messages().Get("no landables"));
				message.clear();
				UI::PlaySound(UI::UISound::FAILURE);
			}
			else if(!target->GetPlanet()->CanLand())
			{
				message = "The authorities on this " + target->GetPlanet()->Noun() +
					" refuse to clear you to land here.";
				messageCategory = GameData::MessageCategories().Get("high");
				UI::PlaySound(UI::UISound::FAILURE);
			}
			else if(!types.empty())
			{
				message = "You can land on more than one ";
				set<string>::const_iterator it = types.begin();
				message += *it++;
				if(it != types.end())
				{
					set<string>::const_iterator last = --types.end();
					if(it != last)
						message += ',';
					while(it != last)
						message += ' ' + *it++ + ',';
					message += " or " + *it;
				}
				message += " in this system. Landing on " + target->DisplayName() + ".";
			}
			else
				message = "Landing on " + target->DisplayName() + ".";
		}
		if(!message.empty())
			Messages::Add({message, messageCategory});
	}
	else if(activeCommands.Has(Command::JUMP | Command::FLEET_JUMP))
	{
		if(player.TravelPlan().empty() && !isWormhole)
		{
			double bestMatch = -2.;
			const auto &links = (ship.JumpNavigation().HasJumpDrive() ?
				ship.GetSystem()->JumpNeighbors(ship.JumpNavigation().JumpRange()) : ship.GetSystem()->Links());
			for(const System *link : links)
			{
				// Not all systems in range are necessarily visible. Don't allow
				// jumping to systems which haven't been seen.
				if(!player.HasSeen(*link))
					continue;

				Point direction = link->Position() - ship.GetSystem()->Position();
				double match = ship.Facing().Unit().Dot(direction.Unit());
				if(match > bestMatch)
				{
					bestMatch = match;
					ship.SetTargetSystem(link);
				}
			}
		}
		else if(isWormhole)
		{
			// The player is guaranteed to have a travel plan for isWormhole to be true.
			Messages::Add({"Landing on a local wormhole to navigate to the "
					+ player.TravelPlan().back()->DisplayName() + " system.",
					GameData::MessageCategories().Get("normal")});
		}
		if(ship.GetTargetSystem() && !isWormhole)
		{
			string name = "selected star";
			if(player.KnowsName(*ship.GetTargetSystem()))
				name = ship.GetTargetSystem()->DisplayName();

			if(activeCommands.Has(Command::FLEET_JUMP))
			{
				// Note: also has command JUMP on only the first call.
				if(activeCommands.Has(Command::JUMP))
					Messages::Add({"Engaging fleet autopilot to jump to the " + name + " system."
						" Your fleet will jump when ready.",
						GameData::MessageCategories().Get("normal")});
			}
			else
				Messages::Add({"Engaging autopilot to jump to the " + name + " system.",
					GameData::MessageCategories().Get("normal")});
		}
	}
	else if(activeCommands.Has(Command::SCAN))
		command |= Command::SCAN;
	else if(activeCommands.Has(Command::HARVEST))
	{
		OrderSingle newOrder{Orders::Types::HARVEST};
		IssueOrder(newOrder, "preparing to harvest.");
	}
	else if(activeCommands.Has(Command::NEAREST_ASTEROID))
	{
		TargetMinable(ship);
	}

	const shared_ptr<const Ship> target = ship.GetTargetShip();
	auto targetOverride = Preferences::Has("Aim turrets with mouse") ^ activeCommands.Has(Command::AIM_TURRET_HOLD)
		? optional(mousePosition) : nullopt;
	AimTurrets(ship, firingCommands, !Preferences::Has("Turrets focus fire"), targetOverride);
	if(Preferences::GetAutoFire() != Preferences::AutoFire::OFF && !ship.IsBoarding()
			&& !(autoPilot | activeCommands).Has(Command::LAND | Command::JUMP | Command::FLEET_JUMP | Command::BOARD)
			&& (!target || target->GetGovernment()->IsEnemy()))
		AutoFire(ship, firingCommands, false, true);

	const bool mouseTurning = activeCommands.Has(Command::MOUSE_TURNING_HOLD);
	if(mouseTurning && !ship.IsBoarding() && (!ship.IsReversing() || ship.Attributes().Get("reverse thrust")))
		command.SetTurn(TurnToward(ship, mousePosition));

	if(activeCommands)
	{
		if(activeCommands.Has(Command::FORWARD))
			command |= Command::FORWARD;
		if(activeCommands.Has(Command::RIGHT | Command::LEFT) && !mouseTurning)
			command.SetTurn(activeCommands.Has(Command::RIGHT) - activeCommands.Has(Command::LEFT));
		if(activeCommands.Has(Command::BACK))
		{
			if(!activeCommands.Has(Command::FORWARD) && ship.Attributes().Get("reverse thrust"))
				command |= Command::BACK;
			else if(!activeCommands.Has(Command::RIGHT | Command::LEFT | Command::AUTOSTEER))
				command.SetTurn(TurnBackward(ship));
		}

		if(activeCommands.Has(Command::PRIMARY))
		{
			int index = 0;
			for(const Hardpoint &hardpoint : ship.Weapons())
			{
				if(hardpoint.IsReady() && !hardpoint.GetWeapon()->Icon())
					firingCommands.SetFire(index);
				++index;
			}
		}
		if(activeCommands.Has(Command::SECONDARY))
		{
			int index = 0;
			const auto &playerSelectedWeapons = player.SelectedSecondaryWeapons();
			for(const Hardpoint &hardpoint : ship.Weapons())
			{
				if(hardpoint.IsReady() && (playerSelectedWeapons.find(hardpoint.GetOutfit()) != playerSelectedWeapons.end()))
					firingCommands.SetFire(index);
				++index;
			}
		}
		if(activeCommands.Has(Command::AFTERBURNER))
			command |= Command::AFTERBURNER;

		if(activeCommands.Has(AutopilotCancelCommands()))
			autoPilot = activeCommands;
	}
	bool shouldAutoAim = false;
	bool isFiring = activeCommands.Has(Command::PRIMARY) || activeCommands.Has(Command::SECONDARY);
	if(activeCommands.Has(Command::AUTOSTEER) && !command.Turn() && !ship.IsBoarding()
			&& !autoPilot.Has(Command::LAND | Command::JUMP | Command::FLEET_JUMP | Command::BOARD))
	{
		if(target && target->GetSystem() == ship.GetSystem() && target->IsTargetable())
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
		else if(ship.GetTargetAsteroid())
			command.SetTurn(TurnToward(ship, TargetAim(ship, *ship.GetTargetAsteroid())));
		else if(ship.GetTargetStellar())
			command.SetTurn(TurnToward(ship, ship.GetTargetStellar()->Position() - ship.Position()));
	}
	else if((Preferences::GetAutoAim() == Preferences::AutoAim::ALWAYS_ON
			|| (Preferences::GetAutoAim() == Preferences::AutoAim::WHEN_FIRING && isFiring))
			&& !command.Turn() && !ship.IsBoarding()
			&& ((target && target->GetSystem() == ship.GetSystem() && target->IsTargetable()) || ship.GetTargetAsteroid())
			&& !autoPilot.Has(Command::LAND | Command::JUMP | Command::FLEET_JUMP | Command::BOARD))
	{
		// Check if this ship has any forward-facing weapons.
		for(const Hardpoint &weapon : ship.Weapons())
			if(!weapon.CanAim(ship) && !weapon.IsTurret() && weapon.GetOutfit())
			{
				shouldAutoAim = true;
				break;
			}
	}
	if(shouldAutoAim)
	{
		Point pos = (target ? target->Position() : ship.GetTargetAsteroid()->Position());
		if((pos - ship.Position()).Unit().Dot(ship.Facing().Unit()) >= .8)
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
	}

	if(autoPilot.Has(Command::JUMP | Command::FLEET_JUMP) && !(player.HasTravelPlan() || ship.GetTargetSystem()))
	{
		// The player completed their travel plan, which may have indicated a destination within the final system.
		autoPilot.Clear(Command::JUMP | Command::FLEET_JUMP);
		const Planet *planet = player.TravelDestination();
		if(planet && planet->IsInSystem(ship.GetSystem()) && planet->IsAccessible(&ship))
		{
			Messages::Add({"Autopilot: landing on " + planet->DisplayName() + ".",
				GameData::MessageCategories().Get("normal")});
			autoPilot |= Command::LAND;
			ship.SetTargetStellar(ship.GetSystem()->FindStellar(planet));
		}
	}

	// Clear autopilot actions if actions can't be performed.
	if(autoPilot.Has(Command::LAND) && !ship.GetTargetStellar())
		autoPilot.Clear(Command::LAND);
	if(autoPilot.Has(Command::JUMP | Command::FLEET_JUMP) && !(ship.GetTargetSystem() || isWormhole))
		autoPilot.Clear(Command::JUMP | Command::FLEET_JUMP);
	if(autoPilot.Has(Command::BOARD) && !(ship.GetTargetShip() && CanBoard(ship, *ship.GetTargetShip())))
		autoPilot.Clear(Command::BOARD);

	if(autoPilot.Has(Command::LAND) || (autoPilot.Has(Command::JUMP | Command::FLEET_JUMP) && isWormhole))
	{
		if(activeCommands.Has(Command::WAIT) || (autoPilot.Has(Command::FLEET_JUMP) && !EscortsReadyToLand(ship)))
			command |= Command::WAIT;

		if(ship.GetPlanet())
			autoPilot.Clear(Command::LAND | Command::JUMP | Command::FLEET_JUMP);
		else
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
	}
	else if(autoPilot.Has(Command::STOP))
	{
		// STOP is automatically cleared once the ship has stopped.
		if(Stop(ship, command))
			autoPilot.Clear(Command::STOP);
	}
	else if(autoPilot.Has(Command::JUMP | Command::FLEET_JUMP) && !ship.IsEnteringHyperspace())
	{
		if(!ship.JumpNavigation().HasHyperdrive() && !ship.JumpNavigation().HasJumpDrive())
		{
			Messages::Add(*GameData::Messages().Get("no hyperdrive"));
			autoPilot.Clear();
			UI::PlaySound(UI::UISound::FAILURE);
		}
		else if(!ship.JumpNavigation().JumpFuel(ship.GetTargetSystem()))
		{
			Messages::Add(*GameData::Messages().Get("cannot jump"));
			autoPilot.Clear();
			UI::PlaySound(UI::UISound::FAILURE);
		}
		else if(!ship.JumpsRemaining() && !ship.IsEnteringHyperspace())
		{
			Messages::Add(*GameData::Messages().Get("no fuel"));
			autoPilot.Clear();
			UI::PlaySound(UI::UISound::FAILURE);
		}
		else if(ship.IsLanding())
		{
			Messages::Add(*GameData::Messages().Get("cannot jump while landing"));
			autoPilot.Clear(Command::JUMP);
			UI::PlaySound(UI::UISound::FAILURE);
		}
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;

			// Don't jump yet if the player is holding jump key or fleet jump is active and
			// escorts are not ready to jump yet.
			if(activeCommands.Has(Command::WAIT) || (autoPilot.Has(Command::FLEET_JUMP) && !EscortsReadyToJump(ship)))
				command |= Command::WAIT;
		}
	}
	else if(autoPilot.Has(Command::BOARD))
	{
		if(!CanBoard(ship, *target))
			autoPilot.Clear(Command::BOARD);
		else
		{
			MoveTo(ship, command, target->Position(), target->Velocity(), 40., .8);
			command |= Command::BOARD;
		}
	}

	if(ship.HasBays() && HasDeployments(ship))
	{
		command |= Command::DEPLOY;
		Deploy(ship, !Preferences::Has("Damaged fighters retreat"));
	}
	if(isCloaking)
		command |= Command::CLOAK;

	ship.SetCommands(command);
	ship.SetCommands(firingCommands);
}



void AI::DisengageAutopilot()
{
	Messages::Add(*GameData::Messages().Get("disengaging autopilot"));
	autoPilot.Clear();
}



bool AI::Has(const Ship &ship, const weak_ptr<const Ship> &other, int type) const
{
	auto sit = actions.find(ship.shared_from_this());
	if(sit == actions.end())
		return false;

	auto oit = sit->second.find(other);
	if(oit == sit->second.end())
		return false;

	return (oit->second & type);
}



bool AI::Has(const Government *government, const weak_ptr<const Ship> &other, int type) const
{
	auto git = governmentActions.find(government);
	if(git == governmentActions.end())
		return false;

	auto oit = git->second.find(other);
	if(oit == git->second.end())
		return false;

	return (oit->second & type);
}



// True if the ship has committed the action against that government. For
// example, if the player boarded any ship belonging to that government.
bool AI::Has(const Ship &ship, const Government *government, int type) const
{
	auto sit = notoriety.find(ship.shared_from_this());
	if(sit == notoriety.end())
		return false;

	auto git = sit->second.find(government);
	if(git == sit->second.end())
		return false;

	return (git->second & type);
}



void AI::UpdateStrengths(map<const Government *, int64_t> &strength, const System *playerSystem)
{
	// Tally the strength of a government by the strength of its present and able ships.
	governmentRosters.clear();
	for(const auto &it : ships)
		if(it->GetGovernment() && it->GetSystem() == playerSystem)
		{
			governmentRosters[it->GetGovernment()].emplace_back(it.get());
			if(!it->IsDisabled() && !it->IsOverheated() && !it->IsIonized())
				strength[it->GetGovernment()] += it->Strength();
		}

	// Strengths of enemies and allies are rebuilt every step.
	enemyStrength.clear();
	allyStrength.clear();
	for(const auto &gov : strength)
	{
		set<const Government *> allies;
		for(const auto &enemy : strength)
			if(enemy.first->IsEnemy(gov.first))
			{
				// "Know your enemies."
				enemyStrength[gov.first] += enemy.second;
				for(const auto &ally : strength)
					if(ally.first->IsEnemy(enemy.first) && !allies.contains(ally.first))
					{
						// "The enemy of my enemy is my friend."
						allyStrength[gov.first] += ally.second;
						allies.insert(ally.first);
					}
			}
	}

	// Ships with nearby allies consider their allies' strength as well as their own.
	for(const auto &it : ships)
	{
		const Government *gov = it->GetGovernment();

		// Check if this ship's government has the authority to enforce scans & fines in this system.
		if(!scanPermissions.contains(gov))
			scanPermissions.emplace(gov, gov && gov->CanEnforce(playerSystem));

		// Only have ships update their strength estimate once per second on average.
		if(!gov || it->GetSystem() != playerSystem || it->IsDisabled() || Random::Int(60))
			continue;

		int64_t &myStrength = shipStrength[it.get()];
		for(const auto &allies : governmentRosters)
		{
			// If this is not an allied government, its ships will not assist this ship when attacked.
			if(allies.first->AttitudeToward(gov) <= 0.)
				continue;
			for(const auto &ally : allies.second)
				if(!ally->IsDisabled() && ally->Position().Distance(it->Position()) < 2000.)
					myStrength += ally->Strength();
		}
	}
}



// Cache various lists of all targetable ships in the player's system for this Step.
void AI::CacheShipLists()
{
	allyLists.clear();
	enemyLists.clear();
	for(const auto &git : governmentRosters)
	{
		allyLists.emplace(git.first, vector<Ship *>());
		allyLists.at(git.first).reserve(ships.size());
		enemyLists.emplace(git.first, vector<Ship *>());
		enemyLists.at(git.first).reserve(ships.size());
		for(const auto &oit : governmentRosters)
		{
			auto &list = git.first->IsEnemy(oit.first)
					? enemyLists[git.first] : allyLists[git.first];
			list.insert(list.end(), oit.second.begin(), oit.second.end());
		}
	}
}



void AI::RegisterDerivedConditions(ConditionsStore &conditions)
{
	// Special conditions about system hostility.
	conditions["government strength: "].ProvidePrefixed([this](const ConditionEntry &ce) -> int64_t {
		const Government *gov = GameData::Governments().Get(ce.NameWithoutPrefix());
		int64_t strength = 0;
		for(const Ship *ship : governmentRosters[gov])
			if(ship)
				strength += ship->Strength();
		return strength;
	});
	conditions["ally strength"].ProvideNamed([this](const ConditionEntry &ce) -> int64_t {
		return allyStrength[GameData::PlayerGovernment()];
	});
	conditions["enemy strength"].ProvideNamed([this](const ConditionEntry &ce) -> int64_t {
		return enemyStrength[GameData::PlayerGovernment()];
	});
	conditions["ally strength: "].ProvidePrefixed([this](const ConditionEntry &ce) -> int64_t {
		const Government *gov = GameData::Governments().Get(ce.NameWithoutPrefix());
		return gov ? allyStrength[gov] : 0.;
	});
	conditions["enemy strength: "].ProvidePrefixed([this](const ConditionEntry &ce) -> int64_t {
		const Government *gov = GameData::Governments().Get(ce.NameWithoutPrefix());
		return gov ? enemyStrength[gov] : 0.;
	});
}



void AI::IssueOrder(const OrderSingle &newOrder, const string &description)
{
	// Figure out what ships we are giving orders to.
	string who;
	vector<const Ship *> ships;
	size_t destroyedCount = 0;
	// A "hold fire" order can be used on the flagship to temporarily block autofire.
	// Other orders can be issued only to escorts.
	bool includeFlagship = newOrder.type == Orders::Types::HOLD_FIRE;
	const vector<weak_ptr<Ship>> &selected = player.SelectedEscorts();
	if(selected.empty())
	{
		for(const shared_ptr<Ship> &it : player.Ships())
			if((includeFlagship || it.get() != player.Flagship()) && !it->IsParked())
			{
				if(it->IsDestroyed())
					++destroyedCount;
				else
					ships.push_back(it.get());
			}
		who = (ships.empty() ? destroyedCount : ships.size()) > 1
			? "Your fleet is " : includeFlagship ? "Your flagship is " : "Your escort is ";
	}
	else
	{
		for(const weak_ptr<Ship> &it : selected)
		{
			shared_ptr<Ship> ship = it.lock();
			if(!ship)
				continue;
			if(ship->IsDestroyed())
				++destroyedCount;
			else
				ships.push_back(ship.get());
		}
		who = (ships.empty() ? destroyedCount : ships.size()) > 1
			? "The selected escorts are " : "The selected escort is ";
	}
	if(ships.empty())
	{
		if(destroyedCount)
			Messages::Add({who + "destroyed and unable to execute your orders.",
				GameData::MessageCategories().Get("normal")});
		return;
	}

	Point centerOfGravity;
	bool isMoveOrder = newOrder.type == Orders::Types::MOVE_TO;
	int squadCount = 0;
	if(isMoveOrder)
	{
		for(const Ship *ship : ships)
			if(ship->GetSystem() && !ship->IsDisabled())
			{
				centerOfGravity += ship->Position();
				++squadCount;
			}
		if(squadCount > 1)
			centerOfGravity /= squadCount;
	}
	// If this is a move command, make sure the fleet is bunched together
	// enough that each ship takes up no more than about 30,000 square pixels.
	double maxSquadOffset = sqrt(10000. * squadCount);

	const Ship *flagship = player.Flagship();
	const Ship *newTargetShip = newOrder.GetTargetShip().get();
	// A target is valid if we have no target, or when the target is in the
	// same system as the flagship.
	bool isValidTarget = !newTargetShip || newOrder.GetTargetAsteroid()
		|| (flagship && newTargetShip->GetSystem() == flagship->GetSystem());

	// Now, go through all the given ships and add the new order to their sets.
	// But, if it turns out that they already had the given order,
	// this order will be cleared instead. The only command that does not
	// toggle is a move command; it always counts as a new command.
	bool hasMismatch = isMoveOrder;
	bool gaveOrder = false;
	bool alreadyHarvesting = false;
	if(isValidTarget)
	{
		for(const Ship *ship : ships)
		{
			// Never issue orders to a ship to target itself.
			if(ship == newTargetShip)
				continue;

			gaveOrder = true;
			hasMismatch |= !orders.contains(ship);

			OrderSet &existing = orders[ship];
			existing.Add(newOrder, &hasMismatch, &alreadyHarvesting);

			if(isMoveOrder)
			{
				// In a move order, rather than commanding every ship to move to the
				// same point, they move as a mass so their center of gravity is
				// that point but their relative positions are unchanged.
				Point offset = ship->Position() - centerOfGravity;
				if(offset.Length() > maxSquadOffset)
					offset = offset.Unit() * maxSquadOffset;
				existing.SetTargetPoint(existing.GetTargetPoint() + offset);
			}
			else if(existing.Has(Orders::Types::HOLD_POSITION))
			{
				bool shouldReverse = false;
				// Set the point this ship will "guard," so it can return
				// to it if knocked away by projectiles / explosions.
				existing.SetTargetPoint(StoppingPoint(*ship, Point(), shouldReverse));
			}
		}
		if(!gaveOrder)
			return;
	}

	if(alreadyHarvesting)
		return;
	else if(hasMismatch)
		Messages::Add({who + description, GameData::MessageCategories().Get("normal")});
	else
	{
		if(!isValidTarget)
			Messages::Add({who + "unable to and no longer " + description,
				GameData::MessageCategories().Get("normal")});
		else
			Messages::Add({who + "no longer " + description, GameData::MessageCategories().Get("normal")});

		// Clear any orders that are now empty.
		for(const Ship *ship : ships)
		{
			auto it = orders.find(ship);
			if(it != orders.end() && it->second.Empty())
				orders.erase(it);
		}
	}
}



// Change the ship's order based on its current fulfillment of the order.
void AI::UpdateOrders(const Ship &ship)
{
	// This should only be called for ships with orders that can be carried out.
	auto it = orders.find(&ship);
	if(it == orders.end())
		return;

	it->second.Update(ship);
}



// Look for an existing distance map for this combination of inputs before calculating a new one.
RoutePlan AI::GetRoutePlan(const Ship &ship, const System *targetSystem)
{
	// Note: RecacheJumpRoutes will check and reset the value for us.
	if(player.RecacheJumpRoutes())
		routeCache.clear();

	const System *from = ship.GetSystem();
	const Government *gov = ship.GetGovernment();
	const JumpType driveCapability = ship.JumpNavigation().HasJumpDrive() ? JumpType::JUMP_DRIVE : JumpType::HYPERDRIVE;

	// A cached route that could be used for this ship could depend on the wormholes which this ship can
	// travel through. Find the intersection of all known wormhole required attributes and the attributes
	// which this ship satisfies.
	vector<string> wormholeKeys;
	const auto &shipAttributes = ship.Attributes();
	for(const auto &requirement : GameData::UniverseWormholeRequirements())
		if(shipAttributes.Get(requirement) > 0)
			wormholeKeys.emplace_back(requirement);

	auto key = RouteCacheKey(from, targetSystem, gov, ship.JumpNavigation().JumpRange(), driveCapability,
		wormholeKeys);

	RoutePlan route;
	auto it = routeCache.find(key);
	if(it == routeCache.end())
	{
		route = RoutePlan(ship, *targetSystem, ship.IsYours() ? &player : nullptr);
		routeCache.emplace(key, route);
	}
	else
		route = RoutePlan(it->second);

	return route;
}
