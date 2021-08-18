/* AI.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "AI.h"

#include "Audio.h"
#include "Command.h"
#include "DistanceMap.h"
#include "Flotsam.h"
#include "Government.h"
#include "Hardpoint.h"
#include "Mask.h"
#include "Messages.h"
#include "Minable.h"
#include "pi.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "StellarObject.h"
#include "System.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

using namespace std;

namespace {
	// If the player issues any of those commands, then any auto-pilot actions for the player get cancelled
	const Command &AutopilotCancelCommands()
	{
		static const Command cancelers(Command::LAND | Command::JUMP | Command::BOARD | Command::AFTERBURNER
			| Command::BACK | Command::FORWARD | Command::LEFT | Command::RIGHT | Command::STOP);
		
		return cancelers;
	}
	
	bool IsStranded(const Ship &ship)
	{
		return ship.GetSystem() && !ship.IsEnteringHyperspace() && !ship.GetSystem()->HasFuelFor(ship)
			&& ship.JumpFuel() && ship.Attributes().Get("fuel capacity") && !ship.JumpsRemaining();
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
		for(const weak_ptr<Ship> &escort : ship.GetEscorts())
		{
			shared_ptr<const Ship> locked = escort.lock();
			if(locked && !locked->IsDisabled() && !locked->CanBeCarried()
					&& locked->GetSystem() == ship.GetSystem()
					&& locked->JumpFuel() && !locked->IsReadyToJump(true))
				return false;
		}
		return true;
	}
	
	// Determine if the ship has any usable weapons.
	bool IsArmed(const Ship &ship)
	{
		for(const Hardpoint &hardpoint : ship.Weapons())
		{
			const Weapon *weapon = hardpoint.GetOutfit();
			if(weapon && !hardpoint.IsAntiMissile())
			{
				if(weapon->Ammo() && !ship.OutfitCount(weapon->Ammo()))
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
		
		// First, check if the player selected any carreid ships.
		for(const weak_ptr<Ship> &it : player.SelectedShips())
		{
			shared_ptr<Ship> ship = it.lock();
			if(ship && ship->IsYours() && isCandidate(ship))
				(ship->HasDeployOrder() ? toRecall : toDeploy).emplace_back(ship.get());
		}
		// If needed, check the player's fleet for deployable ships.
		if(toDeploy.empty() && toRecall.empty())
			for(const shared_ptr<Ship> &ship : player.Ships())
				if(isCandidate(ship) && ship.get() != player.Flagship())
					(ship->HasDeployOrder() ? toRecall : toDeploy).emplace_back(ship.get());
		
		// If any ships were not yet ordered to deployed, deploy them.
		if(!toDeploy.empty())
		{
			for(Ship *ship : toDeploy)
				ship->SetDeployOrder(true);
			Messages::Add("Deployed " + to_string(toDeploy.size()) + " carried ships.", Messages::Importance::High);
		}
		// Otherwise, instruct the carried ships to return to their berth.
		else if(!toRecall.empty())
		{
			for(Ship *ship : toRecall)
				ship->SetDeployOrder(false);
			Messages::Add("Recalled " + to_string(toRecall.size()) + " carried ships", Messages::Importance::High);
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
	bool ShouldRefuel(const Ship &ship, const DistanceMap &route, double fuelCapacity = 0.)
	{
		if(!fuelCapacity)
			fuelCapacity = ship.Attributes().Get("fuel capacity");
		
		const System *from = ship.GetSystem();
		const bool systemHasFuel = from->HasFuelFor(ship) && fuelCapacity;
		// If there is no fuel capacity in this ship, no fuel in this
		// system, if it is fully fueled, or its drive doesn't require
		// fuel, then it should not refuel before traveling.
		if(!systemHasFuel || ship.Fuel() == 1. || !ship.JumpFuel())
			return false;
		
		// Calculate the fuel needed to reach the next system with fuel.
		double fuel = fuelCapacity * ship.Fuel();
		const System *to = route.Route(from);
		while(to && !to->HasFuelFor(ship))
			to = route.Route(to);
		
		// The returned system from Route is nullptr when the route is
		// "complete." If 'to' is nullptr here, then there are no fuel
		// stops between the current system (which has fuel) and the
		// desired endpoint system - refuel only if needed.
		return fuel < route.RequiredFuel(from, (to ? to : route.End()));
	}
	
	// Wrapper for ship - target system uses.
	bool ShouldRefuel(const Ship &ship, const System *to)
	{
		if(!to || ship.Fuel() == 1. || !ship.GetSystem()->HasFuelFor(ship))
			return false;
		double fuelCapacity = ship.Attributes().Get("fuel capacity");
		if(!fuelCapacity)
			return false;
		double needed = ship.JumpFuel(to);
		if(needed && to->HasFuelFor(ship))
			return ship.Fuel() * fuelCapacity < needed;
		else
		{
			// If no direct jump route, or the target system has no
			// fuel, perform a more elaborate refueling check.
			return ShouldRefuel(ship, DistanceMap(ship, to), fuelCapacity);
		}
	}
	
	const StellarObject *GetRefuelLocation(const Ship &ship)
	{
		const StellarObject *target = nullptr;
		const System *system = ship.GetSystem();
		if(system)
		{
			// Determine which, if any, planet with fuel is closest.
			double closest = numeric_limits<double>::infinity();
			const Point &p = ship.Position();
			for(const StellarObject &object : system->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && !object.GetPlanet()->IsWormhole()
						&& object.GetPlanet()->HasFuelFor(ship))
				{
					double distance = p.Distance(object.Position());
					if(distance < closest)
					{
						target = &object;
						closest = distance;
					}
				}
		}
		return target;
	}
	
	// Set the ship's TargetStellar or TargetSystem in order to reach the
	// next desired system. Will target a landable planet to refuel.
	void SelectRoute(Ship &ship, const System *targetSystem)
	{
		const System *from = ship.GetSystem();
		if(from == targetSystem || !targetSystem)
			return;
		const DistanceMap route(ship, targetSystem);
		const bool needsRefuel = ShouldRefuel(ship, route);
		const System *to = route.Route(from);
		// The destination may be accessible by both jump and wormhole.
		// Prefer wormhole travel in these cases, to conserve fuel. Must
		// check accessibility as DistanceMap may only see the jump path.
		if(to && !needsRefuel)
			for(const StellarObject &object : from->Objects())
			{
				if(!object.HasSprite() || !object.HasValidPlanet())
					continue;
				
				const Planet &planet = *object.GetPlanet();
				if(planet.IsWormhole() && planet.IsAccessible(&ship) && planet.WormholeDestination(from) == to)
				{
					ship.SetTargetStellar(&object);
					ship.SetTargetSystem(nullptr);
					return;
				}
			}
		else if(needsRefuel)
		{
			// There is at least one planet that can refuel the ship.
			ship.SetTargetStellar(GetRefuelLocation(ship));
			return;
		}
		// Either there is no viable wormhole route to this system, or
		// the target system cannot be reached.
		ship.SetTargetSystem(to);
		ship.SetTargetStellar(nullptr);
	}
	
	const double MAX_DISTANCE_FROM_CENTER = 10000.;
	// Constants for the invisible fence timer.
	const int FENCE_DECAY = 4;
	const int FENCE_MAX = 600;
	// The health remaining before becoming disabled, at which fighters and
	// other ships consider retreating from battle.
	const double RETREAT_HEALTH = .25;
}



AI::AI(const List<Ship> &ships, const List<Minable> &minables, const List<Flotsam> &flotsam)
	: ships(ships), minables(minables), flotsam(flotsam)
{
}



// Fleet commands from the player.
void AI::IssueShipTarget(const PlayerInfo &player, const shared_ptr<Ship> &target)
{
	Orders newOrders;
	bool isEnemy = target->GetGovernment()->IsEnemy();
	newOrders.type = (!isEnemy ? Orders::KEEP_STATION
		: target->IsDisabled() ? Orders::FINISH_OFF : Orders::ATTACK);
	newOrders.target = target;
	string description = (isEnemy ? "focusing fire on" : "following") + (" \"" + target->Name() + "\".");
	IssueOrders(player, newOrders, description);
}



void AI::IssueMoveTarget(const PlayerInfo &player, const Point &target, const System *moveToSystem)
{
	Orders newOrders;
	newOrders.type = Orders::MOVE_TO;
	newOrders.point = target;
	newOrders.targetSystem = moveToSystem;
	string description = "moving to the given location";
	description += player.GetSystem() == moveToSystem ? "." : (" in the " + moveToSystem->Name() + " system.");
	IssueOrders(player, newOrders, description);
}



// Commands issued via the keyboard (mostly, to the flagship).
void AI::UpdateKeys(PlayerInfo &player, Command &activeCommands)
{
	escortsUseAmmo = Preferences::Has("Escorts expend ammo");
	escortsAreFrugal = Preferences::Has("Escorts use ammo frugally");
	
	autoPilot |= activeCommands;
	if(activeCommands.Has(AutopilotCancelCommands()))
	{
		bool canceled = (autoPilot.Has(Command::JUMP) && !activeCommands.Has(Command::JUMP));
		canceled |= (autoPilot.Has(Command::STOP) && !activeCommands.Has(Command::STOP));
		canceled |= (autoPilot.Has(Command::LAND) && !activeCommands.Has(Command::LAND));
		canceled |= (autoPilot.Has(Command::BOARD) && !activeCommands.Has(Command::BOARD));
		if(canceled)
			Messages::Add("Disengaging autopilot.", Messages::Importance::High);
		autoPilot.Clear();
	}
	
	const Ship *flagship = player.Flagship();
	if(!flagship || flagship->IsDestroyed())
		return;
	
	if(activeCommands.Has(Command::STOP))
		Messages::Add("Coming to a stop.", Messages::Importance::High);
	
	// Only toggle the "cloak" command if one of your ships has a cloaking device.
	if(activeCommands.Has(Command::CLOAK))
		for(const auto &it : player.Ships())
			if(!it->IsParked() && it->Attributes().Get("cloak"))
			{
				isCloaking = !isCloaking;
				Messages::Add(isCloaking ? "Engaging cloaking device." : "Disengaging cloaking device."
					, Messages::Importance::High);
				break;
			}
	
	// Toggle your secondary weapon.
	if(activeCommands.Has(Command::SELECT))
		player.SelectNext();
	
	// The commands below here only apply if you have escorts or fighters.
	if(player.Ships().size() < 2)
		return;
	
	// Toggle the "deploy" command for the fleet or selected ships.
	if(activeCommands.Has(Command::DEPLOY))
		IssueDeploy(player);
	
	shared_ptr<Ship> target = flagship->GetTargetShip();
	Orders newOrders;
	if(activeCommands.Has(Command::FIGHT) && target && !target->IsYours())
	{
		newOrders.type = target->IsDisabled() ? Orders::FINISH_OFF : Orders::ATTACK;
		newOrders.target = target;
		IssueOrders(player, newOrders, "focusing fire on \"" + target->Name() + "\".");
	}
	if(activeCommands.Has(Command::HOLD))
	{
		newOrders.type = Orders::HOLD_POSITION;
		IssueOrders(player, newOrders, "holding position.");
	}
	if(activeCommands.Has(Command::GATHER))
	{
		newOrders.type = Orders::GATHER;
		newOrders.target = player.FlagshipPtr();
		IssueOrders(player, newOrders, "gathering around your flagship.");
	}
	
	// Get rid of any invalid orders. Carried ships will retain orders in case they are deployed.
	for(auto it = orders.begin(); it != orders.end(); )
	{
		if(it->second.type & Orders::REQUIRES_TARGET)
		{
			shared_ptr<Ship> ship = it->second.target.lock();
			// Check if the target ship itself is targetable.
			bool invalidTarget = !ship || !ship->IsTargetable() || (ship->IsDisabled() && it->second.type == Orders::ATTACK);
			// Check if the target ship is in a system where we can target.
			// This check only checks for undocked ships (that have a current system).
			bool targetOutOfReach = !ship || (it->first->GetSystem() && ship->GetSystem() != it->first->GetSystem()
					&& ship->GetSystem() != flagship->GetSystem());
			
			if(invalidTarget || targetOutOfReach)
			{
				it = orders.erase(it);
				continue;
			}
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
				event.TargetGovernment()->Offend(newActions, target->RequiredCrew());
			}
		}
	}
}



// Remove records of what happened in the previous system, now that
// the player has entered a new one.
void AI::Clean()
{
	actions.clear();
	notoriety.clear();
	governmentActions.clear();
	scanPermissions.clear();
	playerActions.clear();
	swarmCount.clear();
	fenceCount.clear();
	miningAngle.clear();
	miningTime.clear();
	appeasmentThreshold.clear();
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



void AI::Step(const PlayerInfo &player, Command &activeCommands)
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
		if(it->Position().Length() >= MAX_DISTANCE_FROM_CENTER)
		{
			int &value = fenceCount[&*it];
			value = min(FENCE_MAX, value + FENCE_DECAY + 1);
		}
	
	const Ship *flagship = player.Flagship();
	step = (step + 1) & 31;
	int targetTurn = 0;
	int minerCount = 0;
	const int maxMinerCount = minables.empty() ? 0 : 9;
	bool opportunisticEscorts = !Preferences::Has("Turrets focus fire");
	bool fightersRetreat = Preferences::Has("Damaged fighters retreat");
	for(const auto &it : ships)
	{
		// Skip any carried fighters or drones that are somehow in the list.
		if(!it->GetSystem())
			continue;
		
		if(it.get() == flagship)
		{
			// Player cannot do anything if the flagship is landing.
			if(!flagship->IsLanding())
				MovePlayer(*it, player, activeCommands);
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
			// Derelicts never ask for help (only the player should repair them).
			if(it->IsDestroyed() || it->GetPersonality().IsDerelict())
				continue;
			
			// Attempt to find a friendly ship to render assistance.
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
					double &threshold = appeasmentThreshold[it.get()];
					threshold = max((1. - health) + .1, threshold);
				}
				continue;
			}
		}
		// Overheated ships are effectively disabled, and cannot fire, cloak, etc.
		if(it->IsOverheated())
			continue;
		
		// Special case: if the player's flagship tries to board a ship to
		// refuel it, that escort should hold position for boarding.
		isStranded |= (flagship && it == flagship->GetTargetShip() && CanBoard(*flagship, *it)
			&& autoPilot.Has(Command::BOARD));
		
		Command command;
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
		else if(DoCloak(*it, command))
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
		if(isPresent && !personality.IsSwarming())
		{
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			if(targetTurn == step || !target || target->IsDestroyed() || (target->IsDisabled()
					&& personality.Disables()) || !target->IsTargetable())
				it->SetTargetShip(FindTarget(*it));
		}
		if(isPresent)
		{
			AimTurrets(*it, command, it->IsYours() ? opportunisticEscorts : personality.IsOpportunistic());
			AutoFire(*it, command);
		}
		
		// If this ship is hyperspacing, or in the act of
		// launching or landing, it can't do anything else.
		if(it->IsHyperspacing() || it->Zoom() < 1.)
		{
			it->SetCommands(command);
			continue;
		}
		
		// Special actions when a ship is heavily damaged:
		if(healthRemaining < RETREAT_HEALTH + .1)
		{
			// Cowards abandon their fleets.
			if(parent && personality.IsCoward())
			{
				parent.reset();
				it->SetParent(parent);
			}
			// Appeasing ships jettison cargo to distract their pursuers.
			if(personality.IsAppeasing() && it->Cargo().Used())
			{
				double health = .5 * it->Shields() + it->Hull();
				double &threshold = appeasmentThreshold[it.get()];
				if(1. - health > threshold)
				{
					int toDump = 11 + (1. - health) * .5 * it->Cargo().Size();
					for(const auto &commodity : it->Cargo().Commodities())
						if(commodity.second && toDump > 0)
						{
							int dumped = min(commodity.second, toDump);
							it->Jettison(commodity.first, dumped, true);
							toDump -= dumped;
						}
					Messages::Add(gov->GetName() + " " + it->Noun() + " \"" + it->Name()
						+ "\": Please, just take my cargo and leave me alone.", Messages::Importance::High);
					threshold = (1. - health) + .1;
				}
			}
		}
		
		// If recruited to assist a ship, follow through on the commitment
		// instead of ignoring it due to other personality traits.
		shared_ptr<Ship> shipToAssist = it->GetShipToAssist();
		if(shipToAssist)
		{
			if(shipToAssist->IsDestroyed() || shipToAssist->GetSystem() != it->GetSystem()
					|| shipToAssist->IsLanding() || shipToAssist->IsHyperspacing()
					|| shipToAssist->GetGovernment()->IsEnemy(gov)
					|| (!shipToAssist->IsDisabled() && shipToAssist->JumpsRemaining()))
			{
				shipToAssist.reset();
				it->SetShipToAssist(shipToAssist);
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
				continue;
			}
		}
		
		// This ship may have updated its target ship.
		double targetDistance = numeric_limits<double>::infinity();
		target = it->GetTargetShip();
		if(target)
			targetDistance = target->Position().Distance(it->Position());
		
		// Behave in accordance with personality traits.
		if(isPresent && personality.IsSwarming() && !isStranded)
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
			continue;
		}
		
		// Surveillance NPCs with enforcement authority (or those from
		// missions) should perform scans and surveys of the system.
		if(isPresent && personality.IsSurveillance() && !isStranded
				&& (scanPermissions[gov] || it->IsSpecial()))
		{
			DoSurveillance(*it, command, target);
			it->SetCommands(command);
			continue;
		}
		
		// Ships that harvest flotsam prioritize it over stopping to be refueled.
		if(isPresent && personality.Harvests() && DoHarvesting(*it, command))
		{
			it->SetCommands(command);
			continue;
		}
		
		// Attacking a hostile ship and stopping to be refueled are more important than mining.
		if(isPresent && personality.IsMining() && !target && !isStranded && maxMinerCount)
		{
			// Miners with free cargo space and available mining time should mine. Mission NPCs
			// should mine even if there are other miners or they have been mining a while.
			if(it->Cargo().Free() >= 5 && IsArmed(*it) && (it->IsSpecial()
					|| (++miningTime[&*it] < 3600 && ++minerCount < maxMinerCount)))
			{
				if(it->HasBays())
				{
					command |= Command::DEPLOY;
					Deploy(*it, false);
				}
				DoMining(*it, command);
				it->SetCommands(command);
				continue;
			}
			// Fighters and drones should assist their parent's mining operation if they cannot
			// carry ore, and the asteroid is near enough that the parent can harvest the ore.
			const shared_ptr<Minable> &minable = parent ? parent->GetTargetAsteroid() : nullptr;
			if(it->CanBeCarried() && parent && miningTime[&*parent] < 3601 && minable
					&& minable->Position().Distance(parent->Position()) < 600.)
			{
				it->SetTargetAsteroid(minable);
				MoveToAttack(*it, command, *minable);
				AutoFire(*it, command, *minable);
				it->SetCommands(command);
				continue;
			}
			else
				it->SetTargetAsteroid(nullptr);
		}
		
		// Handle carried ships:
		if(it->CanBeCarried())
		{
			// A carried ship must belong to the same government as its parent to dock with it.
			bool hasParent = parent && !parent->IsDestroyed() && parent->GetGovernment() == gov;
			bool inParentSystem = hasParent && parent->GetSystem() == it->GetSystem();
			bool parentHasSpace = inParentSystem && parent->BaysFree(it->Attributes().Category());
			if(!hasParent || (!inParentSystem && !it->JumpFuel()) || (!parentHasSpace && !Random::Int(1800)))
			{
				// Find the possible parents for orphaned fighters and drones.
				auto parentChoices = vector<shared_ptr<Ship>>{};
				parentChoices.reserve(ships.size() * .1);
				auto getParentFrom = [&it, &gov, &parentChoices](const list<shared_ptr<Ship>> otherShips) -> shared_ptr<Ship>
				{
					for(const auto &other : otherShips)
						if(other->GetGovernment() == gov && other->GetSystem() == it->GetSystem() && !other->CanBeCarried())
						{
							if(!other->IsDisabled() && other->CanCarry(*it.get()))
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
				
				it->SetParent(parent);
			}
			// Otherwise, check if this ship wants to return to its parent (e.g. to repair).
			else if(parentHasSpace && ShouldDock(*it, *parent, playerSystem))
			{
				it->SetTargetShip(parent);
				MoveTo(*it, command, parent->Position(), parent->Velocity(), 40., .8);
				command |= Command::BOARD;
				it->SetCommands(command);
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
		if(mustRecall || isStranded)
		{
			// Stopping to let fighters board or to be refueled takes priority
			// even over following orders from the player.
			if(it->Velocity().Length() > .001 || !target)
				Stop(*it, command);
			else
				command.SetTurn(TurnToward(*it, TargetAim(*it)));
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
		else if(parent->Commands().Has(Command::JUMP) && it->JumpsRemaining())
			MoveEscort(*it, command);
		// Timid ships always stay near their parent. Injured player
		// escorts will stay nearby until they have repaired a bit.
		else if((personality.IsTimid() || (it->IsYours() && healthRemaining < RETREAT_HEALTH))
				&& parent->Position().Distance(it->Position()) > 500.)
			MoveEscort(*it, command);
		// Otherwise, attack targets depending on how heroic you are.
		else if(target && (targetDistance < 2000. || personality.IsHeroic()))
			MoveIndependent(*it, command);
		// This ship does not feel like fighting.
		else
			MoveEscort(*it, command);
		
		// Force ships that are overlapping each other to "scatter":
		DoScatter(*it, command);
		
		it->SetCommands(command);
	}
}



// Get the in-system strength of each government's allies and enemies.
int64_t AI::AllyStrength(const Government *government)
{
	auto it = allyStrength.find(government);
	return (it == allyStrength.end() ? 0 : it->second);
}



int64_t AI::EnemyStrength(const Government *government)
{
	auto it = enemyStrength.find(government);
	return (it == enemyStrength.end() ? 0 : it->second);
}



// Check if the given target can be pursued by this ship.
bool AI::CanPursue(const Ship &ship, const Ship &target) const
{
	// If this ship does not care about the "invisible fence", it can always pursue.
	if(ship.GetPersonality().IsUnconstrained())
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
	if(HasHelper(ship, isStranded))
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
				bool harmless = helper->IsDisabled() || (helper->IsOverheated() && helper->Heat() >= 1.1) || !helper->IsTargetable();
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
			if(helper->IsYours() && ship.IsYours() && orders.count(helper.get()))
				continue;
			
			// Check if this ship is physically able to help.
			if(!CanHelp(ship, *helper, isStranded))
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
bool AI::CanHelp(const Ship &ship, const Ship &helper, const bool needsFuel)
{
	// Fighters, drones, and disabled / absent ships can't offer assistance.
	if(helper.CanBeCarried() || helper.GetSystem() != ship.GetSystem()
			|| (helper.Cloaking() == 1. && helper.GetGovernment() != ship.GetGovernment())
			|| helper.IsDisabled() || helper.IsOverheated() || helper.IsHyperspacing())
		return false;
	
	// An enemy cannot provide assistance, and only ships of the same government will repair disabled ships.
	if(helper.GetGovernment()->IsEnemy(ship.GetGovernment())
			|| (ship.IsDisabled() && helper.GetGovernment() != ship.GetGovernment()))
		return false;
	
	// If the helper has insufficient fuel, it cannot help this ship unless this ship is also disabled.
	if(!ship.IsDisabled() && needsFuel && !helper.CanRefuel(ship))
		return false;
	
	return true;
}



bool AI::HasHelper(const Ship &ship, const bool needsFuel)
{
	// Do we have an existing ship that was asked to assist?
	if(helperList.find(&ship) != helperList.end())
	{
		shared_ptr<Ship> helper = helperList[&ship].lock();
		if(helper && helper->GetShipToAssist().get() == &ship && CanHelp(ship, *helper, needsFuel))
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
		return target;
	
	bool isYours = ship.IsYours();
	if(isYours)
	{
		auto it = orders.find(&ship);
		if(it != orders.end() && (it->second.type == Orders::ATTACK || it->second.type == Orders::FINISH_OFF))
			return it->second.target.lock();
	}
	
	// If this ship is not armed, do not make it fight.
	double minRange = numeric_limits<double>::infinity();
	double maxRange = 0.;
	for(const Hardpoint &weapon : ship.Weapons())
		if(weapon.GetOutfit() && !weapon.IsAntiMissile())
		{
			minRange = min(minRange, weapon.GetOutfit()->Range());
			maxRange = max(maxRange, weapon.GetOutfit()->Range());
		}
	if(!maxRange)
		return target;
	
	const Personality &person = ship.GetPersonality();
	shared_ptr<Ship> oldTarget = ship.GetTargetShip();
	if(oldTarget && !oldTarget->IsTargetable())
		oldTarget.reset();
	if(oldTarget && person.IsTimid() && oldTarget->IsDisabled()
			&& ship.Position().Distance(oldTarget->Position()) > 1000.)
		oldTarget.reset();
	// Ships with 'plunders' personality always destroy the ships they have boarded.
	if(oldTarget && person.Plunders() && !person.Disables() 
			&& oldTarget->IsDisabled() && Has(ship, oldTarget, ShipEvent::BOARD))
		return oldTarget;
	shared_ptr<Ship> parentTarget;
	bool parentIsEnemy = (ship.GetParent() && ship.GetParent()->GetGovernment()->IsEnemy(gov));
	if(ship.GetParent() && !parentIsEnemy)
		parentTarget = ship.GetParent()->GetTargetShip();
	if(parentTarget && !parentTarget->IsTargetable())
		parentTarget.reset();
	
	// Find the closest enemy ship (if there is one). If this ship is "heroic,"
	// it will attack any ship in system. Otherwise, if all its weapons have a
	// range higher than 2000, it will engage ships up to 50% beyond its range.
	// If a ship has short range weapons and is not heroic, it will engage any
	// ship that is within 3000 of it.
	double closest = person.IsHeroic() ? numeric_limits<double>::infinity() :
		(minRange > 1000.) ? maxRange * 1.5 : 4000.;
	bool isDisabled = false;
	bool hasNemesis = false;
	bool canPlunder = person.Plunders() && ship.Cargo().Free();
	// Figure out how strong this ship is.
	int64_t maxStrength = 0;
	auto strengthIt = shipStrength.find(&ship);
	if(!person.IsHeroic() && strengthIt != shipStrength.end())
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
		if(foe == oldTarget || foe == parentTarget)
			range -= 500.;
		
		// Unless this ship is "heroic", it should not chase much stronger ships.
		if(maxStrength && range > 1000. && !foe->IsDisabled())
		{
			const auto otherStrengthIt = shipStrength.find(foe.get());
			if(otherStrengthIt != shipStrength.end() && otherStrengthIt->second > maxStrength)
				continue;
		}
		
		// Ships which only disable never target already-disabled ships.
		if((person.Disables() || (!person.IsNemesis() && foe != oldTarget))
				&& foe->IsDisabled() && !canPlunder)
			continue;
		
		// Ships that don't (or can't) plunder strongly prefer active targets.
		if(!canPlunder)
			range += 5000. * foe->IsDisabled();
		// While those that do, do so only if no "live" enemies are nearby.
		else
			range += 2000. * (2 * foe->IsDisabled() - !Has(ship, foe, ShipEvent::BOARD));
		
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
			target = foe;
			isDisabled = foe->IsDisabled();
			hasNemesis = isPotentialNemesis;
		}
	}
	
	// With no hostile targets, NPCs with enforcement authority (and any
	// mission NPCs) should consider friendly targets for surveillance.
	if(!isYours && !target && (ship.IsSpecial() || scanPermissions.at(gov)))
	{
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		if(cargoScan || outfitScan)
		{
			closest = numeric_limits<double>::infinity();
			const auto allies = GetShipsList(ship, false);
			for(const auto &it : allies)
				if(it->GetGovernment() != gov)
				{
					// Scan friendly ships that are as-yet unscanned by this ship's government.
					if((!cargoScan || Has(gov, it, ShipEvent::SCAN_CARGO))
							&& (!outfitScan || Has(gov, it, ShipEvent::SCAN_OUTFITS)))
						continue;
					
					double range = it->Position().Distance(ship.Position());
					if(range < closest)
					{
						closest = range;
						target = it;
					}
				}
		}
	}
	
	// Run away if your hostile target is not disabled and you are badly damaged.
	// Player ships never stop targeting hostiles, while hostile mission NPCs will
	// do so only if they are allowed to leave.
	if(!isYours && target && target->GetGovernment()->IsEnemy(gov) && !isDisabled
			&& (person.IsFleeing() || (ship.Health() < RETREAT_HEALTH && !person.IsHeroic()
				&& !person.IsStaying() && !parentIsEnemy)))
	{
		// Make sure the ship has somewhere to flee to.
		const System *system = ship.GetSystem();
		if(ship.JumpsRemaining() && (!system->Links().empty() || ship.Attributes().Get("jump drive")))
			target.reset();
		else
			for(const StellarObject &object : system->Objects())
				if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasSpaceport()
						&& object.GetPlanet()->CanLand(ship))
				{
					target.reset();
					break;
				}
	}
	
	// Vindictive personalities without in-range hostile targets keep firing at an old
	// target (instead of perhaps moving about and finding one that is still alive).
	if(!target && person.IsVindictive())
	{
		target = ship.GetTargetShip();
		if(target && (target->Cloaking() == 1. || target->GetSystem() != ship.GetSystem()))
			target.reset();
	}
	
	return target;
}



// Return a list of all targetable ships in the same system as the player that
// match the desired hostility (i.e. enemy or non-enemy). Does not consider the
// ship's current target, as its inclusion may or may not be desired.
vector<shared_ptr<Ship>> AI::GetShipsList(const Ship &ship, bool targetEnemies, double maxRange) const
{
	if(maxRange < 0.)
		maxRange = numeric_limits<double>::infinity();
	
	auto targets = vector<shared_ptr<Ship>>();
	
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



bool AI::FollowOrders(Ship &ship, Command &command) const
{
	auto it = orders.find(&ship);
	if(it == orders.end())
		return false;
	
	int type = it->second.type;
	
	// If your parent is jumping or absent, that overrides your orders unless
	// your orders are to hold position.
	shared_ptr<Ship> parent = ship.GetParent();
	if(parent && type != Orders::HOLD_POSITION && type != Orders::HOLD_ACTIVE && type != Orders::MOVE_TO)
	{
		if(parent->GetSystem() != ship.GetSystem())
			return false;
		if(parent->Commands().Has(Command::JUMP) && ship.JumpsRemaining())
			return false;
	}
	
	shared_ptr<Ship> target = it->second.target.lock();
	if(type == Orders::MOVE_TO && it->second.targetSystem && ship.GetSystem() != it->second.targetSystem)
	{
		// The desired position is in a different system. Find the best
		// way to reach that system (via wormhole or jumping). This may
		// result in the ship landing to refuel.
		SelectRoute(ship, it->second.targetSystem);
		
		// Travel there even if your parent is not planning to travel.
		if(ship.GetTargetSystem())
			MoveIndependent(ship, command);
		else
			return false;
	}
	else if((type == Orders::MOVE_TO || type == Orders::HOLD_ACTIVE) && ship.Position().Distance(it->second.point) > 20.)
		MoveTo(ship, command, it->second.point, Point(), 10., .1);
	else if(type == Orders::HOLD_POSITION || type == Orders::HOLD_ACTIVE || type == Orders::MOVE_TO)
	{
		if(ship.Velocity().Length() > .001 || !ship.GetTargetShip())
			Stop(ship, command);
		else
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
	}
	else if(!target)
	{
		// Note: in AI::UpdateKeys() we already made sure that if a set of orders
		// has a target, the target is in-system and targetable. But, to be sure:
		return false;
	}
	else if(type == Orders::KEEP_STATION)
		KeepStation(ship, command, *target);
	else if(type == Orders::GATHER)
		CircleAround(ship, command, *target);
	else
		MoveIndependent(ship, command);
	
	return true;
}



void AI::MoveIndependent(Ship &ship, Command &command) const
{
	shared_ptr<const Ship> target = ship.GetTargetShip();
	// NPCs should not be beyond the "fence" unless their target is
	// fairly close to it (or they are intended to be there).
	if(!ship.IsYours() && !ship.GetPersonality().IsUnconstrained())
	{
		if(target)
		{
			Point extrapolated = target->Position() + 120. * (target->Velocity() - ship.Velocity());
			if(extrapolated.Length() >= MAX_DISTANCE_FROM_CENTER)
			{
				MoveTo(ship, command, Point(), Point(), 40., .8);
				if(ship.Velocity().Dot(ship.Position()) > 0.)
					command |= Command::FORWARD;
				return;
			}
		}
		else if(ship.Position().Length() >= MAX_DISTANCE_FROM_CENTER)
		{
			// This ship should not be beyond the fence.
			MoveTo(ship, command, Point(), Point(), 40, .8);
			return;
		}
	}
	
	bool friendlyOverride = false;
	if(ship.IsYours())
	{
		auto it = orders.find(&ship);
		if(it != orders.end() && it->second.target.lock() == target)
			friendlyOverride = (it->second.type == Orders::ATTACK || it->second.type == Orders::FINISH_OFF);
	}
	const Government *gov = ship.GetGovernment();
	if(target && (gov->IsEnemy(target->GetGovernment()) || friendlyOverride))
	{
		bool shouldBoard = ship.Cargo().Free() && ship.GetPersonality().Plunders();
		bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
		if(shouldBoard && target->IsDisabled() && !hasBoarded)
		{
			if(ship.IsBoarding())
				return;
			MoveTo(ship, command, target->Position(), target->Velocity(), 40., .8);
			command |= Command::BOARD;
		}
		else
			Attack(ship, command, *target);
		return;
	}
	else if(target)
	{
		// An AI ship that is targeting a non-hostile ship should scan it, or move on.
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		if((!cargoScan || Has(gov, target, ShipEvent::SCAN_CARGO))
				&& (!outfitScan || Has(gov, target, ShipEvent::SCAN_OUTFITS)))
			target.reset();
		else
		{
			CircleAround(ship, command, *target);
			if(!ship.IsYours() && (ship.IsSpecial() || scanPermissions.at(gov)))
				command |= Command::SCAN;
		}
		return;
	}
	
	// A ship has restricted movement options if it is 'staying' or is hostile to its parent.
	const bool shouldStay = ship.GetPersonality().IsStaying()
			|| (ship.GetParent() && ship.GetParent()->GetGovernment()->IsEnemy(gov));
	// Ships should choose a random system/planet for travel if they do not
	// already have a system/planet in mind, and are free to move about.
	const System *origin = ship.GetSystem();
	if(!ship.GetTargetSystem() && !ship.GetTargetStellar() && !shouldStay)
	{
		int jumps = ship.JumpsRemaining(false);
		// Each destination system has an average priority of 10.
		// If you only have one jump left, landing should be high priority.
		int planetWeight = jumps ? (1 + 40 / jumps) : 1;
		
		vector<int> systemWeights;
		int totalWeight = 0;
		const set<const System *> &links = ship.Attributes().Get("jump drive")
			? origin->JumpNeighbors(ship.JumpRange()) : origin->Links();
		if(jumps)
		{
			for(const System *link : links)
			{
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
			if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->HasSpaceport()
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
}



void AI::MoveEscort(Ship &ship, Command &command) const
{
	const Ship &parent = *ship.GetParent();
	bool hasFuelCapacity = ship.Attributes().Get("fuel capacity") && ship.JumpFuel();
	bool isStaying = ship.GetPersonality().IsStaying() || !hasFuelCapacity;
	bool parentIsHere = (ship.GetSystem() == parent.GetSystem());
	// Check if the parent has a target planet that is in the parent's system.
	const Planet *parentPlanet = (parent.GetTargetStellar() ? parent.GetTargetStellar()->GetPlanet() : nullptr);
	bool planetIsHere = (parentPlanet && parentPlanet->IsInSystem(parent.GetSystem()));
	bool systemHasFuel = hasFuelCapacity && ship.GetSystem()->HasFuelFor(ship);
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
		
		if(!ship.GetTargetStellar() && !ship.GetTargetSystem())
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
		DistanceMap distance(ship, parent.GetTargetSystem());
		const System *dest = distance.Route(ship.GetSystem());
		ship.SetTargetSystem(dest);
		if(!dest)
			// This ship has no route to the parent's destination system, so protect it until it jumps away.
			KeepStation(ship, command, parent);
		else if(ShouldRefuel(ship, dest))
			Refuel(ship, command);
		else if(!ship.JumpsRemaining())
			// Return to the system center to maximize solar collection rate.
			MoveTo(ship, command, Point(), Point(), 40., 0.1);
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			if(!(parent.IsEnteringHyperspace() || parent.IsReadyToJump()) || !EscortsReadyToJump(ship))
				command |= Command::WAIT;
		}
	}
	// If an escort is out of fuel, they should refuel without waiting for the
	// "parent" to land (because the parent may not be planning on landing).
	else if(systemHasFuel && !ship.JumpsRemaining())
		Refuel(ship, command);
	else if(parent.Commands().Has(Command::LAND) && parentIsHere && planetIsHere && parentPlanet->CanLand(ship))
	{
		ship.SetTargetSystem(nullptr);
		ship.SetTargetStellar(parent.GetTargetStellar());
		if(parent.IsLanding() || parent.CanLand())
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
		else
			KeepStation(ship, command, parent);
	}
	else if(parent.Commands().Has(Command::BOARD) && parent.GetTargetShip().get() == &ship)
		Stop(ship, command, .2);
	else
		KeepStation(ship, command, parent);
}



// Prefer your parent's target planet for refueling, but if it and your current
// target planet can't fuel you, try to find one that can.
void AI::Refuel(Ship &ship, Command &command)
{
	const StellarObject *parentTarget = (ship.GetParent() ? ship.GetParent()->GetTargetStellar() : nullptr);
	if(CanRefuel(ship, parentTarget))
		ship.SetTargetStellar(parentTarget);
	else if(!CanRefuel(ship, ship.GetTargetStellar()))
		ship.SetTargetStellar(GetRefuelLocation(ship));

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
	// retreating and attacking when at exactly 25% health by adding hysteresis to the check.
	double minHealth = RETREAT_HEALTH + .1 * !ship.Commands().Has(Command::DEPLOY);
	if(ship.Health() < minHealth && (!ship.IsYours() || Preferences::Has("Damaged fighters retreat")))
		return true;
	
	// TODO: Reboard if in need of ammo.
	
	// If a carried ship has fuel capacity but is very low, it should return if
	// the parent can refuel it.
	double maxFuel = ship.Attributes().Get("fuel capacity");
	if(maxFuel && ship.Fuel() < .005 && parent.JumpFuel() < parent.Fuel() *
			parent.Attributes().Get("fuel capacity") - maxFuel)
		return true;
	
	// If an out-of-combat NPC carried ship is carrying a significant cargo
	// load and can transfer some of it to the parent, it should do so.
	if(!ship.IsYours())
	{
		bool hasEnemy = ship.GetTargetShip() && ship.GetTargetShip()->GetGovernment()->IsEnemy(ship.GetGovernment());
		if(!hasEnemy)
		{
			const CargoHold &cargo = ship.Cargo();
			// Mining ships only mine while they have 5 or more free space. While mining, carried ships
			// do not consider docking unless their parent is far from a targetable asteroid.
			if(parent.Cargo().Free() && !cargo.IsEmpty() && cargo.Size() && cargo.Free() < 5)
				return true;
		}
	}
	
	return false;
}



double AI::TurnBackward(const Ship &ship)
{
	return TurnToward(ship, -ship.Velocity());
}



double AI::TurnToward(const Ship &ship, const Point &vector)
{
	Point facing = ship.Facing().Unit();
	double cross = vector.Cross(facing);
	
	if(vector.Dot(facing) > 0.)
	{
		double angle = asin(min(1., max(-1., cross / vector.Length()))) * TO_DEG;
		if(fabs(angle) <= ship.TurnRate())
			return -angle / ship.TurnRate();
	}
	
	bool left = cross < 0.;
	return left - !left;
}



bool AI::MoveToPlanet(Ship &ship, Command &command)
{
	if(!ship.GetTargetStellar())
		return false;
	
	const Point &target = ship.GetTargetStellar()->Position();
	return MoveTo(ship, command, target, Point(), ship.GetTargetStellar()->Radius(), 1.);
}



// Instead of moving to a point with a fixed location, move to a moving point (Ship = position + velocity)
bool AI::MoveTo(Ship &ship, Command &command, const Point &targetPosition, const Point &targetVelocity, double radius, double slow)
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

	bool isFacing = (dp.Unit().Dot(angle.Unit()) > .95);
	if(!isClose || (!isFacing && !shouldReverse))
		command.SetTurn(TurnToward(ship, dp));
	if(isFacing)
		command |= Command::FORWARD;
	else if(shouldReverse)
	{
		command.SetTurn(TurnToward(ship, velocity));
		command |= Command::BACK;
	}
	
	return false;
}



bool AI::Stop(Ship &ship, Command &command, double maxSpeed, const Point direction)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	
	double speed = velocity.Length();
	
	// If asked for a complete stop, the ship needs to be going much slower.
	if(speed <= (maxSpeed ? maxSpeed : .001))
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
		double reverseAcceleration = ship.Attributes().Get("reverse thrust") / ship.Mass();
		double reverseTime = (180. - degreesToTurn) / ship.TurnRate();
		reverseTime += speed / reverseAcceleration;
		
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



void AI::PrepareForHyperspace(Ship &ship, Command &command)
{
	bool hasHyperdrive = ship.Attributes().Get("hyperdrive");
	double scramThreshold = ship.Attributes().Get("scram drive");
	bool hasJumpDrive = ship.Attributes().Get("jump drive");
	if(!hasHyperdrive && !hasJumpDrive)
		return;
	
	bool isJump = !hasHyperdrive || !ship.GetSystem()->Links().count(ship.GetTargetSystem());
	
	Point direction = ship.GetTargetSystem()->Position() - ship.GetSystem()->Position();
	if(!isJump && scramThreshold)
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


	
void AI::CircleAround(Ship &ship, Command &command, const Body &target)
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



void AI::Swarm(Ship &ship, Command &command, const Body &target)
{
	Point direction = target.Position() - ship.Position();
	double maxSpeed = ship.MaxVelocity();
	double rendezvousTime = RendezvousTime(direction, target.Velocity(), maxSpeed);
	if(std::isnan(rendezvousTime) || rendezvousTime > 600.)
		rendezvousTime = 600.;
	direction += rendezvousTime * target.Velocity();
	MoveTo(ship, command, target.Position() + direction, .5 * maxSpeed * direction.Unit(), 50., 2.);
}



void AI::KeepStation(Ship &ship, Command &command, const Body &target)
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
	double mass = ship.Mass();
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
	Point drag = ship.Velocity() * (ship.Attributes().Get("drag") / mass);
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



void AI::Attack(Ship &ship, Command &command, const Ship &target)
{
	// First, figure out what your shortest-range weapon is.
	double shortestRange = 4000.;
	bool isArmed = false;
	bool hasAmmo = false;
	double minSafeDistance = 0.;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		const Weapon *weapon = hardpoint.GetOutfit();
		if(weapon && !hardpoint.IsAntiMissile())
		{
			isArmed = true;
			bool hasThisAmmo = (!weapon->Ammo() || ship.OutfitCount(weapon->Ammo()));
			hasAmmo |= hasThisAmmo;
			
			// Exploding weaponry that can damage this ship requires special
			// consideration (while we have the ammo to use the weapon).
			if(hasThisAmmo && weapon->BlastRadius() && !weapon->IsSafe())
				minSafeDistance = max(weapon->BlastRadius() + weapon->TriggerRadius(), minSafeDistance);
			
			// The missile boat AI should be applied at 1000 pixels range if
			// all weapons are homing or turrets, and at 2000 if not.
			double multiplier = (hardpoint.IsHoming() || hardpoint.IsTurret()) ? 1. : .5;
			shortestRange = min(multiplier * weapon->Range(), shortestRange);
		}
	}
	// If this ship was using the missile boat AI to run away and bombard its
	// target from a distance, have it stop running once it is out of ammo. This
	// is not realistic, but it's a whole lot less annoying for the player when
	// they are trying to hunt down and kill the last missile boat in a fleet.
	if(isArmed && !hasAmmo)
		shortestRange = 0.;
	
	// Deploy any fighters you are carrying.
	if(!ship.IsYours() && ship.HasBays())
	{
		command |= Command::DEPLOY;
		Deploy(ship, false);
	}
	
	// If this ship has only long-range weapons, or some weapons have a
	// blast radius, it should keep some distance instead of closing in.
	Point d = (target.Position() + target.Velocity()) - (ship.Position() + ship.Velocity());
	if((minSafeDistance > 0. || shortestRange > 1000.)
			&& d.Length() < max(1.25 * minSafeDistance, .5 * shortestRange))
	{
		// If this ship can use reverse thrusters, consider doing so.
		double reverseSpeed = ship.MaxReverseVelocity();
		if(reverseSpeed && (reverseSpeed >= min(target.MaxVelocity(), ship.MaxVelocity())
				|| target.Velocity().Dot(-d.Unit()) <= reverseSpeed))
		{
			command.SetTurn(TurnToward(ship, d));
			if(ship.Facing().Unit().Dot(d) >= 0.)
				command |= Command::BACK;
		}
		else
		{
			command.SetTurn(TurnToward(ship, -d));
			if(ship.Facing().Unit().Dot(d) <= 0.)
				command |= Command::FORWARD;
		}
		return;
	}
	
	MoveToAttack(ship, command, target);
}


	
void AI::MoveToAttack(Ship &ship, Command &command, const Body &target)
{
	Point d = target.Position() - ship.Position();
	
	// First of all, aim in the direction that will hit this target.
	command.SetTurn(TurnToward(ship, TargetAim(ship, target)));
	
	// Calculate this ship's "turning radius"; that is, the smallest circle it
	// can make while at full speed.
	double stepsInFullTurn = 360. / ship.TurnRate();
	double circumference = stepsInFullTurn * ship.Velocity().Length();
	double diameter = max(200., circumference / PI);
	
	// If the ship has reverse thrusters and the target is behind it, we can
	// use them to reach the target more quickly.
	if(ship.Facing().Unit().Dot(d.Unit()) < -.75 && ship.Attributes().Get("reverse thrust"))
		command |= Command::BACK;
	// This isn't perfect, but it works well enough.
	else if((ship.Facing().Unit().Dot(d) >= 0. && d.Length() > diameter)
			|| (ship.Velocity().Dot(d) < 0. && ship.Facing().Unit().Dot(d.Unit()) >= .9))
		command |= Command::FORWARD;
	
	// Use an equipped afterburner if possible.
	if(command.Has(Command::FORWARD) && d.Length() < 1000. && ShouldUseAfterburner(ship))
		command |= Command::AFTERBURNER;
}



void AI::PickUp(Ship &ship, Command &command, const Body &target)
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
bool AI::ShouldUseAfterburner(Ship &ship)
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
	if((!neededFuel || fuel - neededFuel > ship.JumpFuel())
			&& (!neededEnergy || neededEnergy / energy < 0.25)
			&& (!outputHeat || ship.Heat() + outputHeat < .9))
		return true;
	
	return false;
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
		for(const shared_ptr<Ship> &other : others)
			if(!other->GetPersonality().IsSwarming())
			{
				// Prefer to swarm ships that are not already being heavily swarmed.
				int count = swarmCount[other.get()] + Random::Int(4);
				if(count < lowestCount)
				{
					target = other;
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



void AI::DoSurveillance(Ship &ship, Command &command, shared_ptr<Ship> &target) const
{
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
		PrepareForHyperspace(ship, command);
		command |= Command::JUMP;
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
		else
			command |= Command::LAND;
	}
	else if(target)
	{
		// Approach and scan the targeted, friendly ship's cargo or outfits.
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		// If the pointer to the target ship exists, it is targetable and in-system.
		bool mustScanCargo = cargoScan && !Has(ship, target, ShipEvent::SCAN_CARGO);
		bool mustScanOutfits = outfitScan && !Has(ship, target, ShipEvent::SCAN_OUTFITS);
		if(!mustScanCargo && !mustScanOutfits)
			ship.SetTargetShip(shared_ptr<Ship>());
		else
		{
			CircleAround(ship, command, *target);
			command |= Command::SCAN;
		}
	}
	else
	{
		const System *system = ship.GetSystem();
		const Government *gov = ship.GetGovernment();
		
		// Consider scanning any non-hostile ship in this system that you haven't yet personally scanned.
		vector<shared_ptr<Ship>> targetShips;
		bool cargoScan = ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan power");
		if(cargoScan || outfitScan)
			for(const auto &grit : governmentRosters)
			{
				if(gov == grit.first || gov->IsEnemy(grit.first))
					continue;
				for(const shared_ptr<Ship> &it : grit.second)
				{
					if((!cargoScan || Has(ship, it, ShipEvent::SCAN_CARGO))
							&& (!outfitScan || Has(ship, it, ShipEvent::SCAN_OUTFITS)))
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
		if(ship.JumpsRemaining(false))
		{
			const auto &links  = ship.Attributes().Get("jump drive") ? system->JumpNeighbors(ship.JumpRange()) : system->Links();
			targetSystems.insert(targetSystems.end(), links.begin(), links.end());
		}
		
		unsigned total = targetShips.size() + targetPlanets.size() + targetSystems.size();
		if(!total)
		{
			// If there is nothing for this ship to scan, have it hold still
			// instead of drifting away from the system center.
			Stop(ship, command);
			return;
		}
		
		unsigned index = Random::Int(total);
		if(index < targetShips.size())
			ship.SetTargetShip(targetShips[index]);
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
	bool isNew = !miningAngle.count(&ship);
	Angle &angle = miningAngle[&ship];
	if(isNew)
		angle = Angle::Random();
	angle += Angle::Random(1.) - Angle::Random(1.);
	double miningRadius = ship.GetSystem()->AsteroidBelt() * pow(2., angle.Unit().X());
	
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
			AutoFire(ship, command, *target);
			return;
		}
	}
	
	Point heading = Angle(30.).Rotate(ship.Position().Unit() * miningRadius) - ship.Position();
	command.SetTurn(TurnToward(ship, heading));
	if(ship.Velocity().Dot(heading.Unit()) < .7 * ship.MaxVelocity())
		command |= Command::FORWARD;
}



bool AI::DoHarvesting(Ship &ship, Command &command)
{
	// If the ship has no target to pick up, do nothing.
	shared_ptr<Flotsam> target = ship.GetTargetFlotsam();
	if(target && ship.Cargo().Free() < target->UnitSize())
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
			if(ship.Cargo().Free() < it->UnitSize())
				continue;
			// Only pick up flotsam that is nearby and that you are facing toward.
			Point p = it->Position() - ship.Position();
			double range = p.Length();
			if(range > 800. || (range > 100. && p.Unit().Dot(ship.Facing().Unit()) < .9))
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
bool AI::DoCloak(Ship &ship, Command &command)
{
	if(ship.Attributes().Get("cloak"))
	{
		// Never cloak if it will cause you to be stranded.
		const Outfit &attributes = ship.Attributes();
		double fuelCost = attributes.Get("cloaking fuel") + attributes.Get("fuel consumption") - attributes.Get("fuel generation");
		if(attributes.Get("cloaking fuel") && !attributes.Get("ramscoop"))
		{
			double fuel = ship.Fuel() * attributes.Get("fuel capacity");
			int steps = ceil((1. - ship.Cloaking()) / attributes.Get("cloak"));
			// Only cloak if you will be able to fully cloak and also maintain it
			// for as long as it will take you to reach full cloak.
			fuel -= fuelCost * (1 + 2 * steps);
			if(fuel < ship.JumpFuel())
				return false;
		}
		
		// If your parent has chosen to cloak, cloak and rendezvous with them.
		const shared_ptr<const Ship> &parent = ship.GetParent();
		if(parent && parent->Commands().Has(Command::CLOAK) && parent->GetSystem() == ship.GetSystem()
				&& !parent->GetGovernment()->IsEnemy(ship.GetGovernment()))
		{
			command |= Command::CLOAK;
			KeepStation(ship, command, *parent);
			return true;
		}
		
		// Otherwise, always cloak if you are in imminent danger.
		static const double MAX_RANGE = 10000.;
		double range = MAX_RANGE;
		shared_ptr<const Ship> nearestEnemy;
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
		bool cloakFreely = (fuelCost <= 0.) && !ship.GetShipToAssist();
		// If this ship is injured / repairing, it should cloak while under threat.
		bool cloakToRepair = (ship.Health() < RETREAT_HEALTH + hysteresis)
				&& (attributes.Get("shield generation") || attributes.Get("hull repair rate"));
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
				if(ship.GetPersonality().IsUnconstrained() || !fenceCount.count(&ship))
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
	}
	return false;
}



void AI::DoScatter(Ship &ship, Command &command)
{
	if(!command.Has(Command::FORWARD))
		return;
	
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
		
		// Move away from this ship. What side of me is it on?
		command.SetTurn(offset.Cross(ship.Facing().Unit()) > 0. ? 1. : -1.);
		return;
	}
}



// Instead of coming to a full stop, adjust to a target velocity vector
Point AI::StoppingPoint(const Ship &ship, const Point &targetVelocity, bool &shouldReverse)
{
	Point position = ship.Position();
	Point velocity = ship.Velocity() - targetVelocity;
	Angle angle = ship.Facing();
	double acceleration = ship.Acceleration();
	double turnRate = ship.TurnRate();
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
		double reverseAcceleration = ship.Attributes().Get("reverse thrust") / ship.Mass();
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
		const Weapon *weapon = hardpoint.GetOutfit();
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
void AI::AimTurrets(const Ship &ship, Command &command, bool opportunistic) const
{
	// First, get the set of potential hostile ships.
	auto targets = vector<const Body *>();
	const Ship *currentTarget = ship.GetTargetShip().get();
	if(opportunistic || !currentTarget || !currentTarget->IsTargetable())
	{
		// Find the maximum range of any of this ship's turrets.
		double maxRange = 0.;
		for(const Hardpoint &weapon : ship.Weapons())
			if(weapon.CanAim())
				maxRange = max(maxRange, weapon.GetOutfit()->Range());
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
				targets.emplace_back(foe.get());
		// Even if the ship's current target ship is beyond maxRange,
		// or is already disabled, consider aiming at it.
		if(currentTarget && currentTarget->IsTargetable()
				&& find(targets.cbegin(), targets.cend(), currentTarget) == targets.cend())
			targets.push_back(currentTarget);
	}
	else
		targets.push_back(currentTarget);
	// If this ship is mining, consider aiming at its target asteroid.
	if(ship.GetTargetAsteroid())
		targets.push_back(ship.GetTargetAsteroid().get());
	
	// If there are no targets to aim at, opportunistic turrets should sweep
	// back and forth at random, with the sweep centered on the "outward-facing"
	// angle. Focused turrets should just point forward.
	if(targets.empty() && !opportunistic)
	{
		for(const Hardpoint &hardpoint : ship.Weapons())
			if(hardpoint.CanAim())
			{
				// Get the index of this weapon.
				int index = &hardpoint - &ship.Weapons().front();
				double offset = (hardpoint.HarmonizedAngle() - hardpoint.GetAngle()).Degrees();
				command.SetAim(index, offset / hardpoint.GetOutfit()->TurretTurn());
			}
		return;
	}
	if(targets.empty())
	{
		for(const Hardpoint &hardpoint : ship.Weapons())
			if(hardpoint.CanAim())
			{
				// Get the index of this weapon.
				int index = &hardpoint - &ship.Weapons().front();
				// First, check if this turret is currently in motion. If not,
				// it only has a small chance of beginning to move.
				double previous = ship.Commands().Aim(index);
				if(!previous && (Random::Int(60)))
					continue;
				
				Angle centerAngle = Angle(hardpoint.GetPoint());
				double bias = (centerAngle - hardpoint.GetAngle()).Degrees() / 180.;
				double acceleration = Random::Real() - Random::Real() + bias;
				command.SetAim(index, previous + .1 * acceleration);
			}
		return;
	}
	// Each hardpoint should aim at the target that it is "closest" to hitting.
	for(const Hardpoint &hardpoint : ship.Weapons())
		if(hardpoint.CanAim())
		{
			// This is where this projectile fires from. Add some randomness
			// based on how skilled the pilot is.
			Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
			start += ship.GetPersonality().Confusion();
			// Get the turret's current facing, in absolute coordinates:
			Angle aim = ship.Facing() + hardpoint.GetAngle();
			// Get this projectile's average velocity.
			const Weapon *weapon = hardpoint.GetOutfit();
			double vp = weapon->WeightedVelocity() + .5 * weapon->RandomVelocity();
			// Loop through each body this hardpoint could shoot at. Find the
			// one that is the "best" in terms of how many frames it will take
			// to aim at it and for a projectile to hit it.
			double bestScore = numeric_limits<double>::infinity();
			double bestAngle = 0.;
			for(const Body *target : targets)
			{
				Point p = target->Position() - start;
				Point v = target->Velocity();
				// Only take the ship's velocity into account if this weapon
				// does not have its own acceleration.
				if(!weapon->Acceleration())
					v -= ship.Velocity();
				// By the time this action is performed, the target will
				// have moved forward one time step.
				p += v;
				
				// Find out how long it would take for this projectile to reach the target.
				double rendezvousTime = RendezvousTime(p, v, vp);
				// If there is no intersection (i.e. the turret is not facing the target),
				// consider this target "out-of-range" but still targetable.
				if(std::isnan(rendezvousTime))
					rendezvousTime = max(p.Length() / (vp ? vp : 1.), 2 * weapon->TotalLifetime());
				
				// Determine where the target will be at that point.
				p += v * rendezvousTime;
				
				// Determine how much the turret must turn to face that vector.
				double degrees = (Angle(p) - aim).Degrees();
				double turnTime = fabs(degrees) / weapon->TurretTurn();
				// All bodies within weapons range have the same basic
				// weight. Outside that range, give them lower priority.
				rendezvousTime = max(0., rendezvousTime - weapon->TotalLifetime());
				// Always prefer targets that you are able to hit.
				double score = turnTime + (180. / weapon->TurretTurn()) * rendezvousTime;
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
				command.SetAim(index, bestAngle / weapon->TurretTurn());
			}
		}
}



// Fire whichever of the given ship's weapons can hit a hostile target.
void AI::AutoFire(const Ship &ship, Command &command, bool secondary) const
{
	const Personality &person = ship.GetPersonality();
	if(person.IsPacifist() || ship.CannotAct())
		return;
	
	bool beFrugal = (ship.IsYours() && !escortsUseAmmo);
	if(person.IsFrugal() || (ship.IsYours() && escortsAreFrugal && escortsUseAmmo))
	{
		// Frugal ships only expend ammunition if they have lost 50% of shields
		// or hull, or if they are outgunned.
		beFrugal = (ship.Hull() + ship.Shields() > 1.5);
		auto ait = allyStrength.find(ship.GetGovernment());
		auto eit = enemyStrength.find(ship.GetGovernment());
		if(ait != allyStrength.end() && eit != enemyStrength.end() && ait->second < eit->second)
			beFrugal = false;
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
		if(it != orders.end() && it->second.target.lock() == currentTarget)
		{
			disabledOverride = (it->second.type == Orders::FINISH_OFF);
			friendlyOverride = disabledOverride | (it->second.type == Orders::ATTACK);
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
	for(const Hardpoint &weapon : ship.Weapons())
		if(weapon.IsReady()
				&& !(!currentTarget && weapon.IsHoming() && weapon.GetOutfit()->Ammo())
				&& !(!secondary && weapon.GetOutfit()->Icon())
				&& !(beFrugal && weapon.GetOutfit()->Ammo())
				&& !(isWaitingToJump && weapon.GetOutfit()->FiringForce()))
			maxRange = max(maxRange, weapon.GetOutfit()->Range());
	// Extend the weapon range slightly to account for velocity differences.
	maxRange *= 1.5;
	
	// Find all enemy ships within range of at least one weapon.
	auto enemies = GetShipsList(ship, true, maxRange);
	// Consider the current target if it is not already considered (i.e. it
	// is a friendly ship and this is a player ship ordered to attack it).
	if(currentTarget && currentTarget->IsTargetable()
			&& find(enemies.cbegin(), enemies.cend(), currentTarget) == enemies.cend())
		enemies.push_back(currentTarget);
	
	int index = -1;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		++index;
		// Skip weapons that are not ready to fire.
		if(!hardpoint.IsReady())
			continue;
		
		const Weapon *weapon = hardpoint.GetOutfit();
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
			if(!secondary || fuel < (isStaying ? 0. : ship.JumpFuel()))
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
			bool hasBoarded = !ship.IsYours() && Has(ship, target, ShipEvent::BOARD);
			if(target->IsDisabled() && (disables || (plunders && !hasBoarded)) && !disabledOverride)
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



void AI::AutoFire(const Ship &ship, Command &command, const Body &target) const
{
	int index = -1;
	for(const Hardpoint &hardpoint : ship.Weapons())
	{
		++index;
		// Only auto-fire primary weapons that take no ammunition.
		if(!hardpoint.IsReady() || hardpoint.GetOutfit()->Icon() || hardpoint.GetOutfit()->Ammo())
			continue;
		
		// Figure out where this weapon will fire from, but add some randomness
		// depending on how accurate this ship's pilot is.
		Point start = ship.Position() + ship.Facing().Rotate(hardpoint.GetPoint());
		start += ship.GetPersonality().Confusion();
		
		const Weapon *weapon = hardpoint.GetOutfit();
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



void AI::MovePlayer(Ship &ship, const PlayerInfo &player, Command &activeCommands)
{
	Command command;
	bool shift = activeCommands.Has(Command::SHIFT);
	
	bool isWormhole = false;
	if(player.HasTravelPlan())
	{
		// Determine if the player is jumping to their target system or landing on a wormhole.
		const System *system = player.TravelPlan().back();
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(object.HasSprite() && object.HasValidPlanet()
				&& object.GetPlanet()->IsAccessible(&ship) && player.HasVisited(*object.GetPlanet())
				&& object.GetPlanet()->WormholeDestination(ship.GetSystem()) == system && player.HasVisited(*system))
			{
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
			// Don't include invisible missions in the check.
			if(!mission.IsVisible())
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
		if(!destinations.empty())
		{
			string message = "Note: you have ";
			message += (missions == 1 ? "a mission that requires" : "missions that require");
			message += " landing on ";
			size_t count = destinations.size();
			bool oxfordComma = (count > 2);
			for(const Planet *planet : destinations)
			{
				message += planet->Name();
				--count;
				if(count > 1)
					message += ", ";
				else if(count == 1)
					message += (oxfordComma ? ", and " : " and ");
			}
			message += " in the system you are jumping to.";
			Messages::Add(message, Messages::Importance::High);
		}
		// If any destination was found, find the corresponding stellar object
		// and set it as your ship's target planet.
		if(bestDestination)
			ship.SetTargetStellar(system->FindStellar(bestDestination));
	}
	
	if(activeCommands.Has(Command::NEAREST))
	{
		// Find the nearest ship to the flagship. If `Shift` is held, consider friendly ships too.
		double closest = numeric_limits<double>::infinity();
		int closeState = 0;
		bool found = false;
		for(const shared_ptr<Ship> &other : ships)
			if(other.get() != &ship && other->IsTargetable())
			{
				// Sort ships into one of three priority states:
				// 0 = friendly, 1 = disabled enemy, 2 = active enemy.
				int state = other->GetGovernment()->IsEnemy(ship.GetGovernment());
				// Do not let "target nearest" select a friendly ship, so that
				// if the player is repeatedly targeting nearest to, say, target
				// a bunch of fighters, they won't start firing on friendly
				// ships as soon as the last one is gone.
				if((!state && !shift) || other->IsYours())
					continue;
				
				state += state * !other->IsDisabled();
				
				double d = other->Position().Distance(ship.Position());
				
				if(state > closeState || (state == closeState && d < closest))
				{
					ship.SetTargetShip(other);
					closest = d;
					closeState = state;
					found = true;
				}
			}
		// If no ship was found, look for nearby asteroids.
		double asteroidRange = 100. * sqrt(ship.Attributes().Get("asteroid scan power"));
		if(!found && asteroidRange)
		{
			for(const shared_ptr<Minable> &asteroid : minables)
			{
				double range = ship.Position().Distance(asteroid->Position());
				if(range < asteroidRange)
				{
					ship.SetTargetAsteroid(asteroid);
					asteroidRange = range;
				}
			}
		}
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
				selectNext = false;
				break;
			}
		}
		if(selectNext)
			ship.SetTargetShip(shared_ptr<Ship>());
	}
	else if(activeCommands.Has(Command::BOARD))
	{
		// Board the nearest disabled ship, focusing on hostiles before allies. Holding
		// `Shift` results in boarding only player-owned escorts in need of assistance.
		shared_ptr<const Ship> target = ship.GetTargetShip();
		if(!target || !CanBoard(ship, *target) || (shift && !target->IsYours()))
		{
			if(shift)
				ship.SetTargetShip(shared_ptr<Ship>());
			
			double closest = numeric_limits<double>::infinity();
			bool foundEnemy = false;
			bool foundAnything = false;
			for(const shared_ptr<Ship> &other : ships)
				if(CanBoard(ship, *other))
				{
					if(shift && !other->IsYours())
						continue;
					
					bool isEnemy = other->GetGovernment()->IsEnemy(ship.GetGovernment());
					double d = other->Position().Distance(ship.Position());
					if((isEnemy && !foundEnemy) || (d < closest && isEnemy == foundEnemy))
					{
						closest = d;
						foundEnemy = isEnemy;
						foundAnything = true;
						ship.SetTargetShip(other);
					}
				}
			if(!foundAnything)
				activeCommands.Clear(Command::BOARD);
		}
	}
	// Player cannot attempt to land while departing from a planet.
	else if(activeCommands.Has(Command::LAND) && !ship.IsEnteringHyperspace() && ship.Zoom() == 1.)
	{
		// Track all possible landable objects in the current system.
		auto landables = vector<const StellarObject *>{};
		
		// If the player is moving slowly over an uninhabited or inaccessible planet,
		// display the default message explaining why they cannot land there.
		string message;
		for(const StellarObject &object : ship.GetSystem()->Objects())
		{
			if(object.HasSprite() && object.HasValidPlanet() && object.GetPlanet()->IsAccessible(&ship))
				landables.emplace_back(&object);
			else if(object.HasSprite())
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < object.Radius() && ship.Velocity().Length() < (MIN_LANDING_VELOCITY / 60.))
					message = object.LandingMessage();
			}
		}
		if(!message.empty())
			Audio::Play(Audio::Get("fail"));
		
		const StellarObject *target = ship.GetTargetStellar();
		// Require that the player's planetary target is one of the current system's planets.
		auto landIt = find(landables.cbegin(), landables.cend(), target);
		if(landIt == landables.cend())
			target = nullptr;
		
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
				Audio::Play(Audio::Get("fail"));
			}
			else if(next != target)
				message = "Switching landing targets. Now landing on " + next->Name() + ".";
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
						if((!planet->CanLand() || !planet->HasSpaceport()) && !planet->IsWormhole())
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
				message = "There are no planets in this system that you can land on.";
				Audio::Play(Audio::Get("fail"));
			}
			else if(!target->GetPlanet()->CanLand())
			{
				message = "The authorities on this " + target->GetPlanet()->Noun() +
					" refuse to clear you to land here.";
				Audio::Play(Audio::Get("fail"));
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
				message += " in this system. Landing on " + target->Name() + ".";
			}
			else
				message = "Landing on " + target->Name() + ".";
		}
		if(!message.empty())
			Messages::Add(message, Messages::Importance::High);
	}
	else if(activeCommands.Has(Command::JUMP))
	{
		if(!ship.GetTargetSystem() && !isWormhole)
		{
			double bestMatch = -2.;
			const auto &links = (ship.Attributes().Get("jump drive") ?
				ship.GetSystem()->JumpNeighbors(ship.JumpRange()) : ship.GetSystem()->Links());
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
			Messages::Add("Landing on a local wormhole to navigate to the "
					+ player.TravelPlan().back()->Name() + " system.", Messages::Importance::High);
		}
		if(ship.GetTargetSystem() && !isWormhole)
		{
			string name = "selected star";
			if(player.KnowsName(*ship.GetTargetSystem()))
				name = ship.GetTargetSystem()->Name();
			
			Messages::Add("Engaging autopilot to jump to the " + name + " system.", Messages::Importance::High);
		}
	}
	else if(activeCommands.Has(Command::SCAN))
		command |= Command::SCAN;
	
	const shared_ptr<const Ship> target = ship.GetTargetShip();
	AimTurrets(ship, command, !Preferences::Has("Turrets focus fire"));
	if(Preferences::Has("Automatic firing") && !ship.IsBoarding()
			&& !(autoPilot | activeCommands).Has(Command::LAND | Command::JUMP | Command::BOARD)
			&& (!target || target->GetGovernment()->IsEnemy()))
		AutoFire(ship, command, false);
	if(activeCommands)
	{
		if(activeCommands.Has(Command::FORWARD))
			command |= Command::FORWARD;
		if(activeCommands.Has(Command::RIGHT | Command::LEFT))
			command.SetTurn(activeCommands.Has(Command::RIGHT) - activeCommands.Has(Command::LEFT));
		if(activeCommands.Has(Command::BACK))
		{
			if(!activeCommands.Has(Command::FORWARD) && ship.Attributes().Get("reverse thrust"))
				command |= Command::BACK;
			else if(!activeCommands.Has(Command::RIGHT | Command::LEFT))
				command.SetTurn(TurnBackward(ship));
		}
		
		if(activeCommands.Has(Command::PRIMARY))
		{
			int index = 0;
			for(const Hardpoint &hardpoint : ship.Weapons())
			{
				if(hardpoint.IsReady() && !hardpoint.GetOutfit()->Icon())
					command.SetFire(index);
				++index;
			}
		}
		if(activeCommands.Has(Command::SECONDARY))
		{
			int index = 0;
			const auto &playerSelectedWeapons = player.SelectedWeapons();
			for(const Hardpoint &hardpoint : ship.Weapons())
			{
				if(hardpoint.IsReady() && (playerSelectedWeapons.find(hardpoint.GetOutfit()) != playerSelectedWeapons.end()))
					command.SetFire(index);
				++index;
			}
		}
		if(activeCommands.Has(Command::AFTERBURNER))
			command |= Command::AFTERBURNER;
		
		if(activeCommands.Has(AutopilotCancelCommands()))
			autoPilot = activeCommands;
	}
	bool shouldAutoAim = false;
	if(Preferences::Has("Automatic aiming") && !command.Turn() && !ship.IsBoarding()
			&& (Preferences::Has("Automatic firing") || activeCommands.Has(Command::PRIMARY))
			&& ((target && target->GetSystem() == ship.GetSystem() && target->IsTargetable())
				|| ship.GetTargetAsteroid())
			&& !autoPilot.Has(Command::LAND | Command::JUMP | Command::BOARD))
	{
		// Check if this ship has any forward-facing weapons.
		for(const Hardpoint &weapon : ship.Weapons())
			if(!weapon.CanAim() && !weapon.IsTurret() && weapon.GetOutfit())
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
	
	if(autoPilot.Has(Command::JUMP) && !(player.HasTravelPlan() || ship.GetTargetSystem()))
	{
		// The player completed their travel plan, which may have indicated a destination within the final system.
		autoPilot.Clear(Command::JUMP);
		const Planet *planet = player.TravelDestination();
		if(planet && planet->IsInSystem(ship.GetSystem()) && planet->IsAccessible(&ship))
		{
			Messages::Add("Autopilot: landing on " + planet->Name() + ".", Messages::Importance::High);
			autoPilot |= Command::LAND;
			ship.SetTargetStellar(ship.GetSystem()->FindStellar(planet));
		}
	}
	
	// Clear autopilot actions if actions can't be performed.
	if(autoPilot.Has(Command::LAND) && !ship.GetTargetStellar())
		autoPilot.Clear(Command::LAND);
	if(autoPilot.Has(Command::JUMP) && !(ship.GetTargetSystem() || isWormhole))
		autoPilot.Clear(Command::JUMP);
	if(autoPilot.Has(Command::BOARD) && !(ship.GetTargetShip() && CanBoard(ship, *ship.GetTargetShip())))
		autoPilot.Clear(Command::BOARD);
	
	if(autoPilot.Has(Command::LAND) || (autoPilot.Has(Command::JUMP) && isWormhole))
	{
		if(ship.GetPlanet())
			autoPilot.Clear(Command::LAND | Command::JUMP);
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
	else if(autoPilot.Has(Command::JUMP))
	{
		if(!ship.Attributes().Get("hyperdrive") && !ship.Attributes().Get("jump drive"))
		{
			Messages::Add("You do not have a hyperdrive installed.", Messages::Importance::Highest);
			autoPilot.Clear();
			Audio::Play(Audio::Get("fail"));
		}
		else if(!ship.JumpFuel(ship.GetTargetSystem()))
		{
			Messages::Add("You cannot jump to the selected system.", Messages::Importance::Highest);
			autoPilot.Clear();
			Audio::Play(Audio::Get("fail"));
		}
		else if(!ship.JumpsRemaining() && !ship.IsEnteringHyperspace())
		{
			Messages::Add("You do not have enough fuel to make a hyperspace jump.", Messages::Importance::Highest);
			autoPilot.Clear();
			Audio::Play(Audio::Get("fail"));
		}
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			if(activeCommands.Has(Command::WAIT))
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
	// Tally the strength of a government by the cost of its present and able ships.
	governmentRosters.clear();
	for(const auto &it : ships)
		if(it->GetGovernment() && it->GetSystem() == playerSystem)
		{
			governmentRosters[it->GetGovernment()].emplace_back(it);
			if(!it->IsDisabled())
				strength[it->GetGovernment()] += it->Cost();
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
					if(ally.first->IsEnemy(enemy.first) && !allies.count(ally.first))
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
		if(!scanPermissions.count(gov))
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
					myStrength += ally->Cost();
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
		allyLists.emplace(git.first, vector<shared_ptr<Ship>>());
		allyLists.at(git.first).reserve(ships.size());
		enemyLists.emplace(git.first, vector<shared_ptr<Ship>>());
		enemyLists.at(git.first).reserve(ships.size());
		for(const auto &oit : governmentRosters)
		{
			auto &list = git.first->IsEnemy(oit.first)
					? enemyLists[git.first] : allyLists[git.first];
			list.insert(list.end(), oit.second.begin(), oit.second.end());
		}
	}
}



void AI::IssueOrders(const PlayerInfo &player, const Orders &newOrders, const string &description)
{
	string who;
	
	// Find out what the target of these orders is.
	const Ship *newTarget = newOrders.target.lock().get();
	
	// Figure out what ships we are giving orders to.
	vector<const Ship *> ships;
	if(player.SelectedShips().empty())
	{
		for(const shared_ptr<Ship> &it : player.Ships())
			if(it.get() != player.Flagship() && !it->IsParked())
				ships.push_back(it.get());
		who = ships.size() > 1 ? "Your fleet is " : "Your escort is ";
	}
	else
	{
		for(const weak_ptr<Ship> &it : player.SelectedShips())
		{
			shared_ptr<Ship> ship = it.lock();
			if(ship)
				ships.push_back(ship.get());
		}
		who = ships.size() > 1 ? "The selected escorts are " : "The selected escort is ";
	}
	// This should never happen, but just in case:
	if(ships.empty())
		return;
	
	Point centerOfGravity;
	bool isMoveOrder = (newOrders.type == Orders::MOVE_TO);
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

	// A target is valid if we have no target, or when the target is in the
	// same system as the flagship.
	bool isValidTarget = !newTarget || (newTarget && player.Flagship() &&
		newTarget->GetSystem() == player.Flagship()->GetSystem());
	
	// Now, go through all the given ships and set their orders to the new
	// orders. But, if it turns out that they already had the given orders,
	// their orders will be cleared instead. The only command that does not
	// toggle is a move command; it always counts as a new command.
	bool hasMismatch = isMoveOrder;
	bool gaveOrder = false;
	if(isValidTarget)
	{
		for(const Ship *ship : ships)
		{
			// Never issue orders to a ship to target itself.
			if(ship == newTarget)
				continue;
			
			gaveOrder = true;
			hasMismatch |= !orders.count(ship);

			Orders &existing = orders[ship];
			// HOLD_ACTIVE cannot be given as manual order, but we make sure here
			// that any HOLD_ACTIVE order also matches when an HOLD_POSITION
			// command is given.
			if(existing.type == Orders::HOLD_ACTIVE)
				existing.type = Orders::HOLD_POSITION;
			
			hasMismatch |= (existing.type != newOrders.type);
			hasMismatch |= (existing.target.lock().get() != newTarget);
			existing = newOrders;
			
			if(isMoveOrder)
			{
				// In a move order, rather than commanding every ship to move to the
				// same point, they move as a mass so their center of gravity is
				// that point but their relative positions are unchanged.
				Point offset = ship->Position() - centerOfGravity;
				if(offset.Length() > maxSquadOffset)
					offset = offset.Unit() * maxSquadOffset;
				existing.point += offset;
			}
			else if(existing.type == Orders::HOLD_POSITION)
			{
				bool shouldReverse = false;
				// Set the point this ship will "guard," so it can return
				// to it if knocked away by projectiles / explosions.
				existing.point = StoppingPoint(*ship, Point(), shouldReverse);
			}
		}
		if(!gaveOrder)
			return;
	}
	if(hasMismatch)
		Messages::Add(who + description, Messages::Importance::High);
	else
	{
		// Clear all the orders for these ships.
		if(!isValidTarget)
			Messages::Add(who + "unable to and no longer " + description, Messages::Importance::High);
		else
			Messages::Add(who + "no longer " + description, Messages::Importance::High);
		
		for(const Ship *ship : ships)
			orders.erase(ship);
	}
}



// Change the ship's order based on its current fulfillment of the order.
void AI::UpdateOrders(const Ship &ship)
{
	// This should only be called for ships with orders that can be carried out.
	auto it = orders.find(&ship);
	if(it == orders.end())
		return;
	
	Orders &order = it->second;
	if((order.type == Orders::MOVE_TO || order.type == Orders::HOLD_ACTIVE) && ship.GetSystem() == order.targetSystem)
	{
		// If nearly stopped on the desired point, switch to a HOLD_POSITION order.
		if(ship.Position().Distance(order.point) < 20. && ship.Velocity().Length() < .001)
			order.type = Orders::HOLD_POSITION;
	}
	else if(order.type == Orders::HOLD_POSITION && ship.Position().Distance(order.point) > 20.)
	{
		// If far from the defined target point, return via a HOLD_ACTIVE order.
		order.type = Orders::HOLD_ACTIVE;
		// Ensure the system reference is maintained.
		order.targetSystem = ship.GetSystem();
	}
}
