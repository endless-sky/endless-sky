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

#include "Armament.h"
#include "Audio.h"
#include "Command.h"
#include "DistanceMap.h"
#include "Flotsam.h"
#include "Government.h"
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
#include "System.h"

#include <SDL2/SDL.h>

#include <cmath>
#include <limits>
#include <set>

using namespace std;

namespace {
	const Command &AutopilotCancelKeys()
	{
		static const Command keys(Command::LAND | Command::JUMP | Command::BOARD | Command::AFTERBURNER
			| Command::BACK | Command::FORWARD | Command::LEFT | Command::RIGHT);
		
		return keys;
	}
	
	bool IsStranded(const Ship &ship)
	{
		return ship.GetSystem() && !ship.GetSystem()->HasFuelFor(ship) && ship.JumpFuel()
			&& ship.Attributes().Get("fuel capacity") && !ship.JumpsRemaining();
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
	
	double AngleDiff(double a, double b)
	{
		a = abs(a - b);
		return min(a, 360. - a);
	}
	
	static const double MAX_DISTANCE_FROM_CENTER = 10000.;
}



AI::AI(const List<Ship> &ships, const List<Minable> &minables, const List<Flotsam> &flotsam)
	: ships(ships), minables(minables), flotsam(flotsam)
{
}


	
// Fleet commands from the player.
void AI::IssueShipTarget(const PlayerInfo &player, const std::shared_ptr<Ship> &target)
{
	Orders newOrders;
	bool isEnemy = target->GetGovernment()->IsEnemy();
	newOrders.type = (!isEnemy ? Orders::GATHER
		: target->IsDisabled() ? Orders::FINISH_OFF : Orders::ATTACK); 
	newOrders.target = target;
	string description = (isEnemy ? "focusing fire on" : "following") + (" \"" + target->Name() + "\".");
	IssueOrders(player, newOrders, description);
}



void AI::IssueMoveTarget(const PlayerInfo &player, const Point &target)
{
	Orders newOrders;
	newOrders.type = Orders::MOVE_TO;
	newOrders.point = target;
	IssueOrders(player, newOrders, "moving to the given location.");
}



// Commands issued via the keyboard (mostly, to the flagship).
void AI::UpdateKeys(PlayerInfo &player, Command &clickCommands, bool isActive)
{
	shift = (SDL_GetModState() & KMOD_SHIFT);
	escortsUseAmmo = Preferences::Has("Escorts expend ammo");
	escortsAreFrugal = Preferences::Has("Escorts use ammo frugally");
	
	Command oldHeld = keyHeld;
	keyHeld.ReadKeyboard();
	keyStuck |= clickCommands;
	clickCommands.Clear();
	keyDown = keyHeld.AndNot(oldHeld);
	if(keyHeld.Has(AutopilotCancelKeys()))
	{
		bool canceled = (keyStuck.Has(Command::JUMP) && !keyHeld.Has(Command::JUMP));
		canceled |= (keyStuck.Has(Command::LAND) && !keyHeld.Has(Command::LAND));
		canceled |= (keyStuck.Has(Command::BOARD) && !keyHeld.Has(Command::BOARD));
		if(canceled)
			Messages::Add("Disengaging autopilot.");
		keyStuck.Clear();
	}
	const Ship *flagship = player.Flagship();
	
	if(!isActive || !flagship || flagship->IsDestroyed())
		return;
	
	++landKeyInterval;
	if(oldHeld.Has(Command::LAND))
		landKeyInterval = 0;
	
	// Only toggle the "cloak" command if one of your ships has a cloaking device.
	if(keyDown.Has(Command::CLOAK))
		for(const auto &it : player.Ships())
			if(!it->IsParked() && it->Attributes().Get("cloak"))
			{
				isCloaking = !isCloaking;
				Messages::Add(isCloaking ? "Engaging cloaking device." : "Disengaging cloaking device.");
				break;
			}
	
	// Toggle your secondary weapon.
	if(keyDown.Has(Command::SELECT))
		player.SelectNext();
	
	// The commands below here only apply if you have escorts or fighters.
	if(player.Ships().size() < 2)
		return;
	
	// Only toggle the "deploy" command if one of your ships has fighter bays.
	if(keyDown.Has(Command::DEPLOY))
		for(const auto &it : player.Ships())
			if(it->HasBays())
			{
				isLaunching = !isLaunching;
				Messages::Add(isLaunching ? "Deploying fighters." : "Recalling fighters.");
				break;
			}
	
	shared_ptr<Ship> target = flagship->GetTargetShip();
	Orders newOrders;
	if(keyDown.Has(Command::FIGHT) && target && !target->IsYours())
	{
		newOrders.type = target->IsDisabled() ? Orders::FINISH_OFF : Orders::ATTACK;
		newOrders.target = target;
		IssueOrders(player, newOrders, "focusing fire on \"" + target->Name() + "\".");
	}
	if(keyDown.Has(Command::HOLD))
	{
		newOrders.type = Orders::HOLD_POSITION;
		IssueOrders(player, newOrders, "holding position.");
	}
	if(keyDown.Has(Command::GATHER))
	{
		newOrders.type = Orders::GATHER;
		newOrders.target = player.FlagshipPtr();
		IssueOrders(player, newOrders, "gathering around your flagship.");
	}
	// Get rid of any invalid orders.
	for(auto it = orders.begin(); it != orders.end(); )
	{
		if(it->second.type & Orders::REQUIRES_TARGET)
		{
			shared_ptr<Ship> ship = it->second.target.lock();
			if(!ship || !ship->IsTargetable() || ship->GetSystem() != it->first->GetSystem()
					|| (ship->IsDisabled() && it->second.type == Orders::ATTACK))
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
		if(event.Actor() && event.Target())
			actions[event.Actor()][event.Target()] |= event.Type();
		if(event.ActorGovernment() && event.Target())
			governmentActions[event.ActorGovernment()][event.Target()] |= event.Type();
		if(event.ActorGovernment()->IsPlayer() && event.Target())
		{
			int &bitmap = playerActions[event.Target()];
			int newActions = event.Type() - (event.Type() & bitmap);
			bitmap |= event.Type();
			// If you provoke the same ship twice, it should have an effect both times.
			if(event.Type() & ShipEvent::PROVOKE)
				newActions |= ShipEvent::PROVOKE;
			event.TargetGovernment()->Offend(newActions, event.Target()->RequiredCrew());
		}
	}
}



void AI::Clean()
{
	actions.clear();
	governmentActions.clear();
	playerActions.clear();
	shipStrength.clear();
	swarmCount.clear();
	miningAngle.clear();
	miningTime.clear();
	appeasmentThreshold.clear();
}



void AI::Step(const PlayerInfo &player)
{
	// First, figure out the comparative strengths of the present governments.
	map<const Government *, int64_t> strength;
	for(const auto &it : ships)
		if(it->GetGovernment() && it->GetSystem() == player.GetSystem() && !it->IsDisabled())
			strength[it->GetGovernment()] += it->Cost();
	enemyStrength.clear();
	allyStrength.clear();
	for(const auto &it : strength)
	{
		set<const Government *> allies;
		for(const auto &eit : strength)
			if(eit.first->IsEnemy(it.first))
			{
				enemyStrength[it.first] += eit.second;
				for(const auto &ait : strength)
					if(ait.first->IsEnemy(eit.first) && !allies.count(ait.first))
					{
						allyStrength[it.first] += ait.second;
						allies.insert(ait.first);
					}
			}
	}
	for(const auto &it : ships)
	{
		const Government *gov = it->GetGovernment();
		// Only have ships update their strength estimate once per second on average.
		if(!gov || it->GetSystem() != player.GetSystem() || it->IsDisabled() || Random::Int(60))
			continue;
		
		int64_t &strength = shipStrength[it.get()];
		for(const auto &oit : ships)
		{
			const Government *ogov = oit->GetGovernment();
			if(!ogov || oit->GetSystem() != player.GetSystem() || oit->IsDisabled())
				continue;
			
			if(ogov->AttitudeToward(gov) > 0. && oit->Position().Distance(it->Position()) < 2000.)
				strength += oit->Cost();
		}
	}		
	
	const Ship *flagship = player.Flagship();
	step = (step + 1) & 31;
	int targetTurn = 0;
	int minerCount = 0;
	for(const auto &it : ships)
	{
		// Skip any carried fighters or drones that are somehow in the list.
		if(!it->GetSystem())
			continue;
		
		if(it.get() == flagship)
		{
			MovePlayer(*it, player);
			continue;
		}
		
		const Government *gov = it->GetGovernment();
		double health = .5 * it->Shields() + it->Hull();
		bool isPresent = (it->GetSystem() == player.GetSystem());
		bool isStranded = IsStranded(*it);
		bool thisIsLaunching = (isLaunching && it->GetSystem() == player.GetSystem());
		if(isStranded || it->IsDisabled())
		{
			if(it->IsDestroyed() || it->GetPersonality().IsDerelict())
				continue;
			
			bool hasEnemy = false;
			Ship *firstAlly = nullptr;
			bool selectNext = false;
			Ship *nextAlly = nullptr;
			for(const auto &ship : ships)
			{
				// Never ask yourself for help.
				if(ship.get() == it.get())
					continue;
				if(ship->IsDisabled() || !ship->IsTargetable() || ship->GetSystem() != it->GetSystem())
					continue;
				// Fighters and drones can't offer assistance.
				if(ship->CanBeCarried())
					continue;
				
				const Government *otherGov = ship->GetGovernment();
				// If any enemies of this ship are in system, it cannot call for help.
				if(otherGov->IsEnemy(gov) && isPresent)
				{
					hasEnemy = true;
					break;
				}
				// Don't ask for help from a ship that is already helping someone.
				if(ship->GetShipToAssist() && ship->GetShipToAssist().get() != it.get())
					continue;
				// Your escorts only help other escorts, and your flagship never helps.
				if((otherGov->IsPlayer() && !gov->IsPlayer()) || ship.get() == flagship)
					continue;
				// Your escorts should not help each other if already under orders.
				if(otherGov->IsPlayer() && gov->IsPlayer() && orders.count(ship.get()))
					continue;
				
				if(it->IsDisabled() ? (otherGov == gov) : (!otherGov->IsEnemy(gov)))
				{
					if(isStranded && !ship->CanRefuel(*it))
						continue;
					
					if(!firstAlly)
						firstAlly = &*ship;
					else if(ship == it)
						selectNext = true;
					else if(selectNext && !nextAlly)
						nextAlly = &*ship;
				}
			}
			
			isStranded = false;
			if(!hasEnemy)
			{
				if(!nextAlly)
					nextAlly = firstAlly;
				if(nextAlly)
				{
					nextAlly->SetShipToAssist(it);
					isStranded = true;
				}
			}
			if(it->IsDisabled())
			{
				// Ships other than escorts should deploy fighters if disabled.
				if(!it->IsYours() || thisIsLaunching)
					it->SetCommands(Command::DEPLOY);
				// Avoid jettisoning cargo as soon as this ship is repaired.
				double &threshold = appeasmentThreshold[it.get()];
				threshold = max((1. - health) + .1, threshold);
				continue;
			}
		}
		// Special case: if the player's flagship tries to board a ship to
		// refuel it, that escort should hold position for boarding.
		isStranded |= (flagship && it == flagship->GetTargetShip() && CanBoard(*flagship, *it)
			&& keyStuck.Has(Command::BOARD));
		
		Command command;
		if(it->IsYours())
		{
			if(thisIsLaunching)
				command |= Command::DEPLOY;
			if(isCloaking)
				command |= Command::CLOAK;
		}
		
		const Personality &personality = it->GetPersonality();
		shared_ptr<Ship> parent = it->GetParent();
		shared_ptr<const Ship> target = it->GetTargetShip();
		
		if(isPresent && personality.IsSwarming())
		{
			parent.reset();
			it->SetParent(parent);
			if(!target || target->IsHyperspacing() || !target->IsTargetable()
					|| target->GetSystem() != it->GetSystem() || !Random::Int(600))
			{
				if(target)
				{
					auto sit = swarmCount.find(target.get());
					if(sit != swarmCount.end() && sit->second > 0)
						--sit->second;
					it->SetTargetShip(shared_ptr<Ship>());
				}
				int lowestCount = 7;
				for(const shared_ptr<Ship> &other : ships)
					if(!other->GetPersonality().IsSwarming() && !other->GetGovernment()->IsEnemy(gov)
							&& other->GetSystem() == it->GetSystem() && other->IsTargetable()
							&& !other->IsHyperspacing())
					{
						int count = swarmCount[other.get()] + Random::Int(4);
						if(count < lowestCount)
						{
							it->SetTargetShip(other);
							lowestCount = count;
						}
					}
				target = it->GetTargetShip();
				if(target)
					++swarmCount[target.get()];
			}
			if(target)
				Swarm(*it, command, *target);
			else if(it->Zoom() == 1.)
				Refuel(*it, command);
			it->SetCommands(command);
			continue;
		}
		
		if(isPresent && personality.IsSurveillance())
		{
			DoSurveillance(*it, command);
			it->SetCommands(command);
			continue;
		}
		// Pick a target and automatically fire weapons.
		if(isPresent)
		{
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			if(targetTurn == step || !target || !target->IsTargetable() || target->IsDestroyed()
					|| (target->IsDisabled() && personality.Disables()))
				it->SetTargetShip(FindTarget(*it));
			
			command |= AutoFire(*it);
		}
		if(isPresent && personality.Harvests() && DoHarvesting(*it, command))
		{
			it->SetCommands(command);
			continue;
		}
		if(isPresent && personality.IsMining() && !target
				&& it->Cargo().Free() >= 5 && ++miningTime[&*it] < 3600 && ++minerCount < 9)
		{
			DoMining(*it, command);
			it->SetCommands(command);
			continue;
		}
		
		// Special actions when a ship is near death:
		if(health < 1.)
		{
			if(parent && personality.IsCoward())
			{
				// Cowards abandon their fleets.
				parent.reset();
				it->SetParent(parent);
			}
			if(personality.IsAppeasing() && it->Cargo().Used())
			{
				double &threshold = appeasmentThreshold[it.get()];
				if(1. - health > threshold)
				{
					// "Appeasing" ships will dump some fraction of their cargo.
					int toDump = 11 + (1. - health) * .5 * it->Cargo().Size();
					for(const auto &commodity : it->Cargo().Commodities())
					{
						it->Jettison(commodity.first, min(commodity.second, toDump));
						toDump -= commodity.second;
						if(toDump <= 0)
							break;
					}
					Messages::Add(it->GetGovernment()->GetName() + " ship \"" + it->Name()
						+ "\": Please, just take my cargo and leave me alone.");
					threshold = (1. - health) + .1;
				}
			}
		}
		
		double targetDistance = numeric_limits<double>::infinity();
		target = it->GetTargetShip();
		if(target)
			targetDistance = target->Position().Distance(it->Position());
		
		// Handle fighters:
		const string &category = it->Attributes().Category();
		bool isFighter = (category == "Fighter");
		if(it->CanBeCarried())
		{
			bool hasSpace = (parent && parent->BaysFree(isFighter) && !parent->GetGovernment()->IsEnemy(gov));
			if(!hasSpace || parent->IsDestroyed() || parent->GetSystem() != it->GetSystem())
			{
				// Handle orphaned fighters and drones.
				parent.reset();
				it->SetParent(parent);
				for(const auto &other : ships)
					if(other->GetGovernment() == gov && !other->IsDisabled()
							&& other->GetSystem() == it->GetSystem() && !other->CanBeCarried() && other->CanCarry(*it.get()))
					{
						parent = other;
						it->SetParent(other);
						if(other->BaysFree(isFighter))
							break;
					}
			}
			else if(parent && !(it->IsYours() ? thisIsLaunching : parent->Commands().Has(Command::DEPLOY)))
			{
				it->SetTargetShip(parent);
				MoveTo(*it, command, parent->Position(), parent->Velocity(), 40., .8);
				command |= Command::BOARD;
				it->SetCommands(command);
				continue;
			}
		}
		bool mustRecall = false;
		if(it->HasBays() && !(it->IsYours() ? thisIsLaunching : it->Commands().Has(Command::DEPLOY)) && !target)
			for(const weak_ptr<const Ship> &ptr : it->GetEscorts())
			{
				shared_ptr<const Ship> escort = ptr.lock();
				if(escort && escort->CanBeCarried() && escort->GetSystem() == it->GetSystem()
						&& !escort->IsDisabled())
				{
					mustRecall = true;
					break;
				}
			}
		
		shared_ptr<Ship> shipToAssist = it->GetShipToAssist();
		if(shipToAssist)
		{
			it->SetTargetShip(shipToAssist);
			if(shipToAssist->IsDestroyed() || shipToAssist->GetSystem() != it->GetSystem()
					|| shipToAssist->IsLanding() || shipToAssist->IsHyperspacing()
					|| (!shipToAssist->IsDisabled() && shipToAssist->JumpsRemaining())
					|| shipToAssist->GetGovernment()->IsEnemy(gov))
				it->SetShipToAssist(shared_ptr<Ship>());
			else if(!it->IsBoarding())
			{
				MoveTo(*it, command, shipToAssist->Position(), shipToAssist->Velocity(), 40., .8);
				command |= Command::BOARD;
			}
			it->SetCommands(command);
			continue;
		}
		
		bool isPlayerEscort = it->IsYours();
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
			// If this is an escort and it has orders to follow, no need for the
			// AI to figure out what action it must perform.
		}
		// Hostile "escorts" (i.e. NPCs that are trailing you) only revert to
		// escort behavior when in a different system from you. Otherwise,
		// the behavior depends on what the parent is doing, whether there
		// are hostile targets nearby, and whether the escort has any
		// immediate needs (like refueling).
		else if(!parent || parent->IsDestroyed() || (parent->IsDisabled() && !isPlayerEscort))
			MoveIndependent(*it, command);
		else if(parent->GetSystem() != it->GetSystem())
		{
			if(personality.IsStaying() || !it->Attributes().Get("fuel capacity"))
				MoveIndependent(*it, command);
			else
				MoveEscort(*it, command);
		}
		// From here down, we're only dealing with ships that have a "parent"
		// which is in the same system as them. If you're an enemy of your
		// "parent," you don't take orders from them.
		else if(personality.IsStaying() || parent->GetGovernment()->IsEnemy(gov))
			MoveIndependent(*it, command);
		// This is a friendly escort. If the parent is getting ready to
		// jump, always follow.
		else if(parent->Commands().Has(Command::JUMP) && it->JumpsRemaining())
			MoveEscort(*it, command);
		// Timid ships always stay near their parent.
		else if(personality.IsTimid() && parent->Position().Distance(it->Position()) > 500.)
			MoveEscort(*it, command);
		// Otherwise, attack targets depending on how heroic you are.
		else if(target && (targetDistance < 2000. || personality.IsHeroic()))
			MoveIndependent(*it, command);
		// This ship does not feel like fighting.
		else
			MoveEscort(*it, command);
		
		// Apply the afterburner if you're in a heated battle and it will not
		// use up your last jump worth of fuel.
		if(it->Attributes().Get("afterburner thrust") && target && !target->IsDisabled()
				&& target->IsTargetable() && target->GetSystem() == it->GetSystem())
		{
			double fuel = it->Fuel() * it->Attributes().Get("fuel capacity");
			if(fuel - it->Attributes().Get("afterburner fuel") >= it->JumpFuel())
				if(command.Has(Command::FORWARD) && targetDistance < 1000.)
					command |= Command::AFTERBURNER;
		}
		// Your own ships cloak on your command; all others do it when the
		// AI considers it appropriate.
		if(!it->IsYours())
			DoCloak(*it, command);
		
		// Force ships that are overlapping each other to "scatter":
		DoScatter(*it, command);
		
		it->SetCommands(command);
	}
}



// Pick a new target for the given ship.
shared_ptr<Ship> AI::FindTarget(const Ship &ship) const
{
	// If this ship has no government, it has no enemies.
	shared_ptr<Ship> target;
	const Government *gov = ship.GetGovernment();
	if(!gov || ship.GetPersonality().IsPacifist())
		return target;
	
	bool isPlayerEscort = ship.IsYours();
	if(isPlayerEscort)
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
	const System *system = ship.GetSystem();
	bool isDisabled = false;
	bool hasNemesis = false;
	// Figure out how strong this ship is.
	int64_t maxStrength = 0;
	auto strengthIt = shipStrength.find(&ship);
	if(!person.IsHeroic() && strengthIt != shipStrength.end())
		maxStrength = 2 * strengthIt->second;
	for(const auto &it : ships)
		if(it->GetSystem() == system && it->IsTargetable() && gov->IsEnemy(it->GetGovernment()))
		{
			// If this is a "nemesis" ship and it has found one of the player's
			// ships to target, it will not go after anything else.
			if(hasNemesis && !it->GetGovernment()->IsPlayer())
				continue;
			
			// Calculate what the range will be a second from now, so that ships
			// will prefer targets that they are headed toward.
			double range = (it->Position() + 60. * it->Velocity()).Distance(
				ship.Position() + 60. * ship.Velocity());
			// Preferentially focus on your previous target or your parent ship's
			// target if they are nearby.
			if(it == oldTarget || it == parentTarget)
				range -= 500.;
			
			// Unless this ship is heroic, it will not chase much stronger ships
			// unless it has strong allies nearby.
			if(maxStrength && range > 1000. && !it->IsDisabled())
			{
				auto otherStrengthIt = shipStrength.find(it.get());
				if(otherStrengthIt != shipStrength.end() && otherStrengthIt->second > maxStrength)
					continue;
			}
			
			// If your personality it to disable ships rather than destroy them,
			// never target disabled ships.
			if(it->IsDisabled() && !person.Plunders()
					&& (person.Disables() || (!person.IsNemesis() && it != oldTarget)))
				continue;
			
			if(!person.Plunders())
				range += 5000. * it->IsDisabled();
			else
			{
				bool hasBoarded = Has(ship, it, ShipEvent::BOARD);
				// Don't plunder unless there are no "live" enemies nearby.
				range += 2000. * (2 * it->IsDisabled() - !hasBoarded);
			}
			// Check if this target has any weapons (not counting anti-missiles).
			bool isArmed = false;
			for(const auto &ait : it->Weapons())
				if(ait.GetOutfit() && !ait.GetOutfit()->AntiMissile())
				{
					isArmed = true;
					break;
				}
			// Prefer to go after armed targets, expecially if you're not a pirate.
			range += 1000. * (!isArmed * (1 + !person.Plunders()));
			// Focus on nearly dead ships.
			range += 500. * (it->Shields() + it->Hull());
			bool isPotentialNemesis = (person.IsNemesis() && it->GetGovernment()->IsPlayer());
			if((isPotentialNemesis && !hasNemesis) || range < closest)
			{
				closest = range;
				target = it;
				isDisabled = it->IsDisabled();
				hasNemesis = isPotentialNemesis;
			}
		}
	
	bool cargoScan = ship.Attributes().Get("cargo scan") || ship.Attributes().Get("cargo scan power");
	bool outfitScan = ship.Attributes().Get("outfit scan") || ship.Attributes().Get("outfit scan power");
	if(!target && (cargoScan || outfitScan) && !isPlayerEscort)
	{
		closest = numeric_limits<double>::infinity();
		for(const auto &it : ships)
			if(it->GetSystem() == system && it->GetGovernment() != gov && it->IsTargetable())
			{
				if((cargoScan && !Has(ship.GetGovernment(), it, ShipEvent::SCAN_CARGO))
						|| (outfitScan && !Has(ship.GetGovernment(), it, ShipEvent::SCAN_OUTFITS)))
				{
					double range = it->Position().Distance(ship.Position());
					if(range < closest)
					{
						closest = range;
						target = it;
					}
				}
			}
	}
	
	// Run away if your target is not disabled and you are badly damaged.
	if(!isDisabled && target && (person.IsFleeing() || 
			(.5 * ship.Shields() + ship.Hull() < 1.
				&& !person.IsHeroic() && !person.IsStaying() && !parentIsEnemy)))
	{
		// Make sure the ship has somewhere to flee to.
		const System *system = ship.GetSystem();
		if(ship.JumpsRemaining() && (!system->Links().empty() || ship.Attributes().Get("jump drive")))
			target.reset();
		else
			for(const StellarObject &object : system->Objects())
				if(object.GetPlanet() && object.GetPlanet()->HasSpaceport()
						&& object.GetPlanet()->CanLand(ship))
				{
					target.reset();
					break;
				}
	}
	if(!target && person.IsVindictive())
	{
		target = ship.GetTargetShip();
		if(target && target->Cloaking() == 1.)
			target.reset();
	}
	
	return target;
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
	if(parent && type != Orders::HOLD_POSITION && type != Orders::MOVE_TO)
	{
		if(parent->GetSystem() != ship.GetSystem())
			return false;
		if(parent->Commands().Has(Command::JUMP) && ship.JumpsRemaining())
			return false;
	}
	
	shared_ptr<Ship> target = it->second.target.lock();
	if(type == Orders::MOVE_TO && ship.Position().Distance(it->second.point) > 20.)
		MoveTo(ship, command, it->second.point, Point(), 10., .1);
	else if(type == Orders::HOLD_POSITION || type == Orders::MOVE_TO)
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
	if(target && !ship.IsYours() && !ship.GetPersonality().IsUnconstrained())
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
	bool friendlyOverride = false;
	if(ship.IsYours())
	{
		auto it = orders.find(&ship);
		if(it != orders.end() && it->second.target.lock() == target)
			friendlyOverride = (it->second.type == Orders::ATTACK || it->second.type == Orders::FINISH_OFF);
	}
	if(target && (ship.GetGovernment()->IsEnemy(target->GetGovernment()) || friendlyOverride))
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
		bool cargoScan = ship.Attributes().Get("cargo scan") || ship.Attributes().Get("cargo scan power");
		bool outfitScan = ship.Attributes().Get("outfit scan") || ship.Attributes().Get("outfit scan power");
		if((!cargoScan || Has(ship.GetGovernment(), target, ShipEvent::SCAN_CARGO))
				&& (!outfitScan || Has(ship.GetGovernment(), target, ShipEvent::SCAN_OUTFITS)))
			target.reset();
		else
		{
			CircleAround(ship, command, *target);
			if(!ship.GetGovernment()->IsPlayer())
				command |= Command::SCAN;
		}
		return;
	}
	
	// If this ship is moving independently because it has a target, not because
	// it has no parent, don't let it make travel plans.
	if(ship.GetParent() && !ship.GetPersonality().IsStaying())
	{
		if(!ship.JumpsRemaining())
			Refuel(ship, command);
		return;
	}
	
	if(!ship.GetTargetSystem() && !ship.GetTargetStellar() && !ship.GetPersonality().IsStaying())
	{
		int jumps = ship.JumpsRemaining();
		// Each destination system has an average priority of 10.
		// If you only have one jump left, landing should be high priority.
		int planetWeight = jumps ? (1 + 40 / jumps) : 1;
		
		vector<int> systemWeights;
		int totalWeight = 0;
		const set<const System *> &links = ship.Attributes().Get("jump drive")
			? ship.GetSystem()->Neighbors() : ship.GetSystem()->Links();
		if(jumps)
		{
			for(const System *link : links)
			{
				// Prefer systems in the direction we're facing.
				Point direction = link->Position() - ship.GetSystem()->Position();
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
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport()
					&& object.GetPlanet()->CanLand(ship))
			{
				planets.push_back(&object);
				totalWeight += planetWeight;
			}
		// If there are no ports to land on and this ship cannot jump, consider
		// landing on uninhabited planets.
		if(!totalWeight)
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.GetPlanet() && object.GetPlanet()->CanLand(ship))
				{
					planets.push_back(&object);
					totalWeight += planetWeight;
				}
		if(!totalWeight)
		{
			// If there is nothing this ship can land on, have it just go to the
			// star and hover over it rather than drifting far away.
			if(ship.GetSystem()->Objects().empty())
				return;
			totalWeight = 1;
			planets.push_back(&ship.GetSystem()->Objects().front());
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
	
	if(ship.GetTargetSystem())
	{
		PrepareForHyperspace(ship, command);
		bool mustWait = false;
		if(ship.BaysFree(false) || ship.BaysFree(true))
			for(const weak_ptr<const Ship> &escort : ship.GetEscorts())
			{
				shared_ptr<const Ship> locked = escort.lock();
				mustWait |= locked && locked->CanBeCarried() && !locked->IsDisabled();
			}
		
		if(!mustWait)
			command |= Command::JUMP;
	}
	else if(ship.GetTargetStellar())
	{
		MoveToPlanet(ship, command);
		if(!ship.GetPersonality().IsStaying() && ship.Attributes().Get("fuel capacity"))
			command |= Command::LAND;
		else if(ship.Position().Distance(ship.GetTargetStellar()->Position()) < 100.)
			ship.SetTargetStellar(nullptr);
	}
	else if(ship.GetPersonality().IsStaying() && ship.GetSystem()->Objects().size())
	{
		unsigned i = Random::Int(ship.GetSystem()->Objects().size());
		ship.SetTargetStellar(&ship.GetSystem()->Objects()[i]);
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
	// If an escort is out of fuel, they should refuel without waiting for the
	// "parent" to land (because the parent may not be planning on landing).
	if(hasFuelCapacity && !ship.JumpsRemaining() && ship.GetSystem()->HasFuelFor(ship))
		Refuel(ship, command);
	else if(!parentIsHere && !isStaying)
	{
		// Check whether the ship has a target system and is able to jump to it.
		bool hasJump = (ship.GetTargetSystem() && ship.JumpFuel(ship.GetTargetSystem()));
		if(!hasJump && !ship.GetTargetStellar())
		{
			// If we're stranded and haven't decided where to go, figure out a
			// path to the parent ship's system.
			DistanceMap distance(ship, parent.GetSystem());
			const System *from = ship.GetSystem();
			const System *to = distance.Route(from);
			for(const StellarObject &object : from->Objects())
				if(object.GetPlanet() && object.GetPlanet()->WormholeDestination(from) == to)
				{
					ship.SetTargetStellar(&object);
					break;
				}
			ship.SetTargetSystem(to);
			// Check if we need to refuel. Wormhole travel does not require fuel.
			if(!ship.GetTargetStellar() && (!to || 
					(from->HasFuelFor(ship) && !to->HasFuelFor(ship) && ship.JumpsRemaining() == 1)))
				Refuel(ship, command);
		}
		// Perform the action that this ship previously decided on.
		if(ship.GetTargetStellar())
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
		else if(ship.GetTargetSystem())
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
		}
	}
	else if(parent.Commands().Has(Command::LAND) && parentIsHere && planetIsHere && parentPlanet->CanLand(ship))
	{
		ship.SetTargetStellar(parent.GetTargetStellar());
		MoveToPlanet(ship, command);
		if(parent.IsLanding() || parent.CanLand())
			command |= Command::LAND;
	}
	else if(parent.Commands().Has(Command::BOARD) && parent.GetTargetShip().get() == &ship)
		Stop(ship, command, .2);
	else if(parent.Commands().Has(Command::JUMP) && parent.GetTargetSystem() && !isStaying)
	{
		DistanceMap distance(ship, parent.GetTargetSystem());
		const System *dest = distance.Route(ship.GetSystem());
		ship.SetTargetSystem(dest);
		if(!dest || (ship.GetSystem()->HasFuelFor(ship) && !dest->HasFuelFor(ship) && ship.JumpsRemaining() == 1))
			Refuel(ship, command);
		else
		{
			PrepareForHyperspace(ship, command);
			if(parent.IsEnteringHyperspace() || parent.IsReadyToJump())
				command |= Command::JUMP;
		}
	}
	else
		KeepStation(ship, command, parent);
}



void AI::Refuel(Ship &ship, Command &command)
{
	const StellarObject *parentTarget = (ship.GetParent() ? ship.GetParent()->GetTargetStellar() : nullptr);
	if(CanRefuel(ship, parentTarget))
		ship.SetTargetStellar(parentTarget);
	else if(!CanRefuel(ship, ship.GetTargetStellar()))
	{
		double closest = numeric_limits<double>::infinity();
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(CanRefuel(ship, &object))
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < closest)
				{
					ship.SetTargetStellar(&object);
					closest = distance;
				}
			}
	}
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
	
	if(!planet->HasSpaceport() || planet->IsWormhole() || !planet->CanLand(ship))
		return false;
	
	return true;
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
	bool isFacing = (dp.Unit().Dot(angle.Unit()) > .8);
	if(!isClose || (!isFacing && !shouldReverse))
		command.SetTurn(TurnToward(ship, dp));
	if(isFacing)
		command |= Command::FORWARD;
	else if(shouldReverse)
		command |= Command::BACK;
	
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


	
void AI::CircleAround(Ship &ship, Command &command, const Ship &target)
{
	Point direction = target.Position() - ship.Position();
	command.SetTurn(TurnToward(ship, direction));
	if(ship.Facing().Unit().Dot(direction) >= 0. && direction.Length() > 200.)
		command |= Command::FORWARD;
}


	
void AI::Swarm(Ship &ship, Command &command, const Ship &target)
{
	Point direction = target.Position() - ship.Position();
	double rendezvousTime = Armament::RendezvousTime(direction, target.Velocity(), ship.MaxVelocity());
	if(rendezvousTime != rendezvousTime || rendezvousTime > 600.)
		rendezvousTime = 600.;
	direction += rendezvousTime * target.Velocity();
	MoveTo(ship, command, target.Position() + direction, Point(), 50., 2.);
}



void AI::KeepStation(Ship &ship, Command &command, const Ship &target)
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
	double positionTime = Armament::RendezvousTime(positionDelta, target.Velocity(), maxV);
	if(positionTime != positionTime || positionTime > MAX_TIME)
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
	if(abs(targetAngle) < turn)
		command.SetTurn(targetAngle / turn);
	else
		command.SetTurn(targetAngle < 0. ? -1. : 1.);
	
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
	for(const Hardpoint &weapon : ship.Weapons())
	{
		const Outfit *outfit = weapon.GetOutfit();
		if(outfit && !weapon.IsAntiMissile())
		{
			isArmed = true;
			if(!outfit->Ammo() || ship.OutfitCount(outfit->Ammo()))
				hasAmmo = true;
			// The missile boat AI should be applied at 1000 pixels range if
			// all weapons are homing or turrets, and at 2000 if not.
			double multiplier = (weapon.IsHoming() || weapon.IsTurret()) ? 1. : .5;
			shortestRange = min(multiplier * outfit->Range(), shortestRange);
		}
	}
	// If this ship was using the missile boat AI to run away and bombard its
	// target from a distance, have it stop running once it is out of ammo. This
	// is not realistic, but it's a whole lot less annoying for the player when
	// they are trying to hunt down and kill the last missile boat in a fleet.
	if(isArmed && !hasAmmo)
		shortestRange = 0.;
	
	// Deploy any fighters you are carrying.
	if(!ship.IsYours())
		command |= Command::DEPLOY;
	// If this ship only has long-range weapons, it should keep its distance
	// instead of trying to close with the target ship.
	Point d = target.Position() - ship.Position();
	if(shortestRange > 1000. && d.Length() < .5 * shortestRange)
	{
		command.SetTurn(TurnToward(ship, -d));
		if(ship.Facing().Unit().Dot(d) <= 0.)
			command |= Command::FORWARD;
		return;
	}
	
	MoveToAttack(ship, command, target);
}


	
void AI::MoveToAttack(Ship &ship, Command &command, const Body &target)
{
	Point d = target.Position() - ship.Position();
	
	// First of all, aim in the direction that will hit this target.
	command.SetTurn(TurnToward(ship, TargetAim(ship, target)));
	
	// Calculate this ship's "turning radius; that is, the smallest circle it
	// can make while at full speed.
	double stepsInFullTurn = 360. / ship.TurnRate();
	double circumference = stepsInFullTurn * ship.Velocity().Length();
	double diameter = max(200., circumference / PI);
	
	// This isn't perfect, but it works well enough.
	if((ship.Facing().Unit().Dot(d) >= 0. && d.Length() > diameter)
			|| (ship.Velocity().Dot(d) < 0. && ship.Facing().Unit().Dot(d.Unit()) >= .9))
		command |= Command::FORWARD;
}



void AI::PickUp(Ship &ship, Command &command, const Body &target)
{
	// Figure out the target's velocity relative to the ship.
	Point p = target.Position() - ship.Position();
	Point v = target.Velocity() - ship.Velocity();
	double vMax = ship.MaxVelocity();
	
	// Estimate where the target will be by the time we reach it.
	double time = Armament::RendezvousTime(p, v, vMax);
	if(std::isnan(time))
		time = p.Length() / vMax;
	double degreesToTurn = TO_DEG * acos(min(1., max(-1., p.Unit().Dot(ship.Facing().Unit()))));
	time += degreesToTurn / ship.TurnRate();
	p += v * time;
	
	// Move toward the target.
	command.SetTurn(TurnToward(ship, p));
	if(p.Unit().Dot(ship.Facing().Unit()) > .7)
		command |= Command::FORWARD;
}



void AI::DoSurveillance(Ship &ship, Command &command) const
{
	const shared_ptr<Ship> &target = ship.GetTargetShip();
	if(target && (!target->IsTargetable() || target->GetSystem() != ship.GetSystem()))
		ship.SetTargetShip(shared_ptr<Ship>());
	if(target && ship.GetGovernment()->IsEnemy(target->GetGovernment()))
	{
		MoveIndependent(ship, command);
		command |= AutoFire(ship);
		return;
	}
	
	bool cargoScan = ship.Attributes().Get("cargo scan") || ship.Attributes().Get("cargo scan power");
	bool outfitScan = ship.Attributes().Get("outfit scan") || ship.Attributes().Get("outfit scan power");
	double atmosphereScan = ship.Attributes().Get("atmosphere scan");
	bool jumpDrive = ship.Attributes().Get("jump drive");
	bool hyperdrive = ship.Attributes().Get("hyperdrive");
	
	// This function is only called for ships that are in the player's system.
	if(ship.GetTargetSystem())
	{
		PrepareForHyperspace(ship, command);
		command |= Command::JUMP;
		command |= Command::DEPLOY;
	}
	else if(ship.GetTargetStellar())
	{
		MoveToPlanet(ship, command);
		double distance = ship.Position().Distance(ship.GetTargetStellar()->Position());
		if(distance < atmosphereScan && !Random::Int(100))
			ship.SetTargetStellar(nullptr);
		else
			command |= Command::LAND;
	}
	else if(ship.GetTargetShip() && ship.GetTargetShip()->IsTargetable()
			&& ship.GetTargetShip()->GetSystem() == ship.GetSystem())
	{
		bool mustScanCargo = cargoScan && !Has(ship, target, ShipEvent::SCAN_CARGO);
		bool mustScanOutfits = outfitScan && !Has(ship, target, ShipEvent::SCAN_OUTFITS);
		bool isInSystem = (ship.GetSystem() == target->GetSystem() && !target->IsEnteringHyperspace());
		if(!isInSystem || (!mustScanCargo && !mustScanOutfits))
			ship.SetTargetShip(shared_ptr<Ship>());
		else
		{
			CircleAround(ship, command, *target);
			command |= Command::SCAN;
		}
	}
	else
	{
		shared_ptr<Ship> newTarget = FindTarget(ship);
		if(newTarget && ship.GetGovernment()->IsEnemy(newTarget->GetGovernment()))
		{
			ship.SetTargetShip(newTarget);
			return;
		}
		
		vector<shared_ptr<Ship>> targetShips;
		vector<const StellarObject *> targetPlanets;
		vector<const System *> targetSystems;
		
		if(cargoScan || outfitScan)
			for(const auto &it : ships)
				if(it->GetGovernment() != ship.GetGovernment() && it->IsTargetable()
						&& it->GetSystem() == ship.GetSystem())
				{
					if(Has(ship, it, ShipEvent::SCAN_CARGO) && Has(ship, it, ShipEvent::SCAN_OUTFITS))
						continue;
				
					targetShips.push_back(it);
				}
		
		if(atmosphereScan)
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(!object.IsStar() && object.Radius() < 130.)
					targetPlanets.push_back(&object);
		
		bool canJump = (ship.JumpsRemaining() != 0);
		if(jumpDrive && canJump)
			for(const System *link : ship.GetSystem()->Neighbors())
				targetSystems.push_back(link);
		else if(hyperdrive && canJump)
			for(const System *link : ship.GetSystem()->Links())
				targetSystems.push_back(link);
		
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
	if(!target)
	{
		for(const shared_ptr<Minable> &minable : minables)
		{
			Point offset = minable->Position() - ship.Position();
			if(offset.Length() < 800. && offset.Unit().Dot(ship.Facing().Unit()) > .7)
			{
				target = minable;
				ship.SetTargetAsteroid(target);
				break;
			}
		}
	}
	if(target)
	{
		MoveToAttack(ship, command, *target);
		command |= AutoFire(ship, *target);
		return;
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
		target.reset();
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
			double time = Armament::RendezvousTime(p, v, vMax);
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
	
	PickUp(ship, command, *target);
	return true;
}



void AI::DoCloak(Ship &ship, Command &command)
{
	if(ship.Attributes().Get("cloak"))
	{
		// Never cloak if it will cause you to be stranded.
		if(ship.Attributes().Get("cloaking fuel") && !ship.Attributes().Get("ramscoop"))
		{
			double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
			fuel -= ship.Attributes().Get("cloaking fuel");
			if(fuel < ship.JumpFuel())
				return;
		}
		// Otherwise, always cloak if you are in imminent danger.
		static const double MAX_RANGE = 10000.;
		double nearestEnemy = MAX_RANGE;
		for(const auto &other : ships)
			if(other->GetSystem() == ship.GetSystem() && other->IsTargetable()
					&& other->GetGovernment()->IsEnemy(ship.GetGovernment())
					&& !other->IsDisabled())
				nearestEnemy = min(nearestEnemy,
					ship.Position().Distance(other->Position()));
		
		// If this ship has started cloaking, it must get at least 40% repaired
		// or 40% farther away before it begins decloaking again.
		double hysteresis = ship.Cloaking() ? 1.4 : 1.;
		double cloakIsFree = !ship.Attributes().Get("cloaking fuel");
		if(ship.Hull() + .5 * ship.Shields() < hysteresis
				&& (cloakIsFree || nearestEnemy < 2000. * hysteresis))
			command |= Command::CLOAK;
		
		// Also cloak if there are no enemies nearby and cloaking does
		// not cost you fuel.
		if(nearestEnemy == MAX_RANGE && cloakIsFree && !ship.GetTargetShip())
			command |= Command::CLOAK;
	}
}



void AI::DoScatter(Ship &ship, Command &command)
{
	if(!command.Has(Command::FORWARD))
		return;
	
	double turnRate = ship.TurnRate();
	double acceleration = ship.Acceleration();
	for(const shared_ptr<Ship> &other : ships)
	{
		if(other.get() == &ship)
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
	const Point &position = ship.Position();
	Point velocity = ship.Velocity() - targetVelocity;
	const Angle &angle = ship.Facing();
	double acceleration = ship.Acceleration();
	double turnRate = ship.TurnRate();
	shouldReverse = false;
	
	// If I were to turn around and stop now the relative movement, where would that put me?
	double v = velocity.Length();
	if(!v)
		return position;
	
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
	return target ? TargetAim(ship, *target) : Point();
}



Point AI::TargetAim(const Ship &ship, const Body &target)
{
	Point result;
	for(const Hardpoint &weapon : ship.Weapons())
	{
		const Outfit *outfit = weapon.GetOutfit();
		if(!outfit || weapon.IsHoming() || weapon.IsTurret())
			continue;
		
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		Point p = target.Position() - start + ship.GetPersonality().Confusion();
		Point v = target.Velocity() - ship.Velocity();
		double steps = Armament::RendezvousTime(p, v, outfit->Velocity() + .5 * outfit->RandomVelocity());
		if(!(steps == steps))
			continue;
		
		steps = min(steps, outfit->TotalLifetime());
		p += steps * v;
		
		double damage = outfit->ShieldDamage() + outfit->HullDamage();
		result += p.Unit() * abs(damage);
	}
	
	return result ? result : target.Position() - ship.Position();
}



// Fire whichever of the given ship's weapons can hit a hostile target.
Command AI::AutoFire(const Ship &ship, bool secondary) const
{
	Command command;
	if(ship.GetPersonality().IsPacifist())
		return command;
	int index = -1;
	
	bool beFrugal = (ship.IsYours() && !escortsUseAmmo);
	if(ship.GetPersonality().IsFrugal() || (ship.IsYours() && escortsAreFrugal && escortsUseAmmo))
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
	// not want to risk damaging that target. The only time a ship other than
	// the player will target a friendly ship is if the player has asked a ship
	// for assistance.
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
	bool spareDisabled = (ship.GetPersonality().Disables() || ship.GetPersonality().Plunders());
	
	// Find the longest range of any of your non-homing weapons.
	double maxRange = 0.;
	for(const Hardpoint &weapon : ship.Weapons())
		if(weapon.IsReady() && !weapon.IsHoming() && (secondary || !weapon.GetOutfit()->Icon()))
			maxRange = max(maxRange, weapon.GetOutfit()->Range());
	// Extend the weapon range slightly to account for velocity differences.
	maxRange *= 1.5;
	
	// Find all enemy ships within range of at least one weapon.
	vector<shared_ptr<const Ship>> enemies;
	if(currentTarget)
		enemies.push_back(currentTarget);
	for(auto target : ships)
		if(target->IsTargetable() && gov->IsEnemy(target->GetGovernment())
				&& !(target->IsHyperspacing() && target->Velocity().Length() > 10.)
				&& target->GetSystem() == ship.GetSystem()
				&& target->Position().Distance(ship.Position()) < maxRange
				&& target != currentTarget)
			enemies.push_back(target);
	
	for(const Hardpoint &weapon : ship.Weapons())
	{
		++index;
		// Skip weapons that are not ready to fire. Also skip homing weapons if
		// no target is selected, and secondary weapons if only firing primaries.
		if(!weapon.IsReady() || (!currentTarget && weapon.IsHoming()))
			continue;
		if(!secondary && weapon.GetOutfit()->Icon())
			continue;
		if(beFrugal && weapon.GetOutfit()->Ammo())
			continue;
		
		// Special case: if the weapon uses fuel, be careful not to spend so much
		// fuel that you cannot leave the system if necessary.
		if(weapon.GetOutfit()->FiringFuel())
		{
			double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
			fuel -= weapon.GetOutfit()->FiringFuel();
			// If the ship is not ever leaving this system, it does not need to
			// reserve any fuel.
			bool isStaying = ship.GetPersonality().IsStaying();
			if(!secondary || fuel < (isStaying ? 0. : ship.JumpFuel()))
				continue;
		}
		// Figure out where this weapon will fire from, but add some randomness
		// depending on how accurate this ship's pilot is.
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		start += ship.GetPersonality().Confusion();
		
		const Outfit *outfit = weapon.GetOutfit();
		double vp = outfit->Velocity() + .5 * outfit->RandomVelocity();
		double lifetime = outfit->TotalLifetime();
		
		if(currentTarget && (weapon.IsHoming() || weapon.IsTurret()))
		{
			bool hasBoarded = Has(ship, currentTarget, ShipEvent::BOARD);
			if(currentTarget->IsDisabled() && spareDisabled && !hasBoarded && !disabledOverride)
				continue;
			// Don't fire turrets at targets that are accelerating or decelerating
			// rapidly due to hyperspace jumping.
			if(weapon.IsTurret() && currentTarget->IsHyperspacing() && currentTarget->Velocity().Length() > 10.)
				continue;
			// Don't fire secondary weapons as targets that have started jumping.
			if(outfit->Icon() && currentTarget->IsEnteringHyperspace())
				continue;
			
			Point p = currentTarget->Position() - start;
			Point v = currentTarget->Velocity() - ship.Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;
			
			if(p.Length() < outfit->BlastRadius())
				continue;
			
			// If this is a homing weapon, it is not necessary to take the
			// velocity of the ship firing it into account.
			if(weapon.IsHoming())
				v = currentTarget->Velocity();
			// Calculate how long it will take the projectile to reach its target.
			double steps = Armament::RendezvousTime(p, v, vp);
			if(steps == steps && steps <= lifetime)
			{
				command.SetFire(index);
				continue;
			}
		}
		// Don't fire homing weapons with no target.
		if(weapon.IsHoming())
			continue;
		
		for(const shared_ptr<const Ship> &target : enemies)
		{
			// Don't shoot ships we want to plunder.
			bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
			if(target->IsDisabled() && spareDisabled && !hasBoarded && !disabledOverride)
				continue;
			
			Point p = target->Position() - start;
			Point v = target->Velocity() - ship.Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;
			
			// Get the vector the weapon will travel along.
			v = (ship.Facing() + weapon.GetAngle()).Unit() * vp - v;
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
	
	return command;
}



Command AI::AutoFire(const Ship &ship, const Body &target) const
{
	Command command;
	
	int index = -1;
	for(const Hardpoint &weapon : ship.Weapons())
	{
		++index;
		// Only auto-fire primary weapons that take no ammunition.
		if(!weapon.IsReady() || weapon.GetOutfit()->Icon() || weapon.GetOutfit()->Ammo())
			continue;
		
		// Figure out where this weapon will fire from, but add some randomness
		// depending on how accurate this ship's pilot is.
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		start += ship.GetPersonality().Confusion();
		
		const Outfit *outfit = weapon.GetOutfit();
		double vp = outfit->Velocity();
		double lifetime = outfit->TotalLifetime();
		
		Point p = target.Position() - start;
		Point v = target.Velocity() - ship.Velocity();
		// By the time this action is performed, the ships will have moved
		// forward one time step.
		p += v;
		
		// Get the vector the weapon will travel along.
		v = (ship.Facing() + weapon.GetAngle()).Unit() * vp - v;
		// Extrapolate over the lifetime of the projectile.
		v *= lifetime;
		
		const Mask &mask = target.GetMask(step);
		if(mask.Collide(-p, v, target.Facing()) < 1.)
			command.SetFire(index);
	}
	return command;
}



void AI::MovePlayer(Ship &ship, const PlayerInfo &player)
{
	Command command;
	
	bool isWormhole = false;
	if(player.HasTravelPlan())
	{
		const System *system = player.TravelPlan().back();
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(object.GetPlanet() && object.GetPlanet()->WormholeDestination(ship.GetSystem()) == system
				&& player.HasVisited(object.GetPlanet()) && player.HasVisited(system))
			{
				isWormhole = true;
				ship.SetTargetStellar(&object);
				break;
			}
		if(!isWormhole)
			ship.SetTargetSystem(system);
	}
	if(ship.IsEnteringHyperspace() && !wasHyperspacing)
	{
		// Check if there's a particular planet there we want to visit.
		const System *system = ship.GetTargetSystem();
		set<const Planet *> destinations;
		Date deadline;
		const Planet *bestDestination = nullptr;
		int count = 0;
		for(const Mission &mission : player.Missions())
		{
			// Don't include invisible missions in the check.
			if(!mission.IsVisible())
				continue;
			
			if(mission.Destination() && mission.Destination()->IsInSystem(system))
			{
				destinations.insert(mission.Destination());
				++count;
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
				if(planet->IsInSystem(system))
				{
					destinations.insert(planet);
					++count;
					if(!bestDestination)
						bestDestination = planet;
				}
		}
		
		// Inform the player of any destinations in the system they are jumping to.
		if(!destinations.empty())
		{
			string message = "Note: you have ";
			message += (count == 1 ? "a mission that requires" : "missions that require");
			message += " landing on ";
			count = destinations.size();
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
			Messages::Add(message);
		}
		// If any destination was found, find the corresponding stellar object
		// and set it as your ship's target planet.
		if(bestDestination)
			ship.SetTargetStellar(system->FindStellar(bestDestination));
	}
	wasHyperspacing = ship.IsEnteringHyperspace();
	
	if(keyDown.Has(Command::NEAREST))
	{
		double closest = numeric_limits<double>::infinity();
		int closeState = 0;
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
				if((!state && !shift) || other->GetGovernment()->IsPlayer())
					continue;
				
				state += state * !other->IsDisabled();
				
				double d = other->Position().Distance(ship.Position());
				
				if(state > closeState || (state == closeState && d < closest))
				{
					ship.SetTargetShip(other);
					closest = d;
					closeState = state;
				}
			}
	}
	else if(keyDown.Has(Command::TARGET))
	{
		shared_ptr<const Ship> target = ship.GetTargetShip();
		bool selectNext = !target || !target->IsTargetable();
		for(const shared_ptr<Ship> &other : ships)
		{
			bool isPlayer = other->GetGovernment()->IsPlayer() || other->GetPersonality().IsEscort();
			if(other == target)
				selectNext = true;
			else if(other.get() != &ship && selectNext && other->IsTargetable() && isPlayer == shift)
			{
				ship.SetTargetShip(other);
				selectNext = false;
				break;
			}
		}
		if(selectNext)
			ship.SetTargetShip(shared_ptr<Ship>());
	}
	else if(keyDown.Has(Command::BOARD))
	{
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
				keyDown.Clear(Command::BOARD);
		}
	}
	else if(keyDown.Has(Command::LAND))
	{
		// If the player is right over an uninhabited planet, display a message
		// explaining why they cannot land there.
		string message;
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(!object.GetPlanet() && object.HasSprite())
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < object.Radius())
					message = object.LandingMessage();
			}
		if(!message.empty())
			Audio::Play(Audio::Get("fail"));
		
		const StellarObject *target = ship.GetTargetStellar();
		if(target && ship.Position().Distance(target->Position()) < target->Radius())
		{
			// Special case: if there are two planets in system and you have one
			// selected, then press "land" again, do not toggle to the other if
			// you are within landing range of the one you have selected.
		}
		else if(message.empty() && target && landKeyInterval < 60)
		{
			bool found = false;
			int count = 0;
			const StellarObject *next = nullptr;
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.GetPlanet() && object.GetPlanet()->IsAccessible(&ship))
				{
					++count;
					if(found)
					{
						next = &object;
						break;
					}
					else if(&object == ship.GetTargetStellar())
						found = true;
				}
			if(!next)
			{
				for(const StellarObject &object : ship.GetSystem()->Objects())
					if(object.GetPlanet() && object.GetPlanet()->IsAccessible(&ship))
					{
						next = &object;
						break;
					}
			}
			ship.SetTargetStellar(next);
			
			if(next->GetPlanet() && !next->GetPlanet()->CanLand())
			{
				message = "The authorities on this " + ship.GetTargetStellar()->GetPlanet()->Noun() +
					" refuse to clear you to land here.";
				Audio::Play(Audio::Get("fail"));
			}
			else if(count > 1)
				message = "Switching landing targets. Now landing on " + next->Name() + ".";
		}
		else if(message.empty())
		{
			double closest = numeric_limits<double>::infinity();
			int count = 0;
			set<string> types;
			if(!target)
			{
				for(const StellarObject &object : ship.GetSystem()->Objects())
					if(object.GetPlanet() && object.GetPlanet()->IsAccessible(&ship))
					{
						++count;
						types.insert(object.GetPlanet()->Noun());
						double distance = ship.Position().Distance(object.Position());
						const Planet *planet = object.GetPlanet();
						if((!planet->CanLand() || !planet->HasSpaceport()) && !planet->IsWormhole())
							distance += 10000.;
					
						if(distance < closest)
						{
							ship.SetTargetStellar(&object);
							closest = distance;
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
				message = "The authorities on this " + ship.GetTargetStellar()->GetPlanet()->Noun() +
					" refuse to clear you to land here.";
				Audio::Play(Audio::Get("fail"));
			}
			else if(count > 1)
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
			Messages::Add(message);
	}
	else if(keyDown.Has(Command::JUMP))
	{
		if(!ship.GetTargetSystem())
		{
			double bestMatch = -2.;
			const auto &links = (ship.Attributes().Get("jump drive") ?
				ship.GetSystem()->Neighbors() : ship.GetSystem()->Links());
			for(const System *link : links)
			{
				Point direction = link->Position() - ship.GetSystem()->Position();
				double match = ship.Facing().Unit().Dot(direction.Unit());
				if(match > bestMatch)
				{
					bestMatch = match;
					ship.SetTargetSystem(link);
				}
			}
		}
		if(ship.GetTargetSystem())
		{
			string name = "selected star";
			if(player.KnowsName(ship.GetTargetSystem()))
				name = ship.GetTargetSystem()->Name();
			
			Messages::Add("Engaging autopilot to jump to the " + name + " system.");
		}
	}
	else if(keyHeld.Has(Command::SCAN))
		command |= Command::SCAN;
	
	bool hasGuns = Preferences::Has("Automatic firing") && !ship.IsBoarding()
		&& !(keyStuck | keyHeld).Has(Command::LAND | Command::JUMP | Command::BOARD)
		&& (!ship.GetTargetShip() || ship.GetTargetShip()->GetGovernment()->IsEnemy());
	if(hasGuns)
		command |= AutoFire(ship, false);
	hasGuns |= keyHeld.Has(Command::PRIMARY);
	if(keyHeld)
	{
		if(keyHeld.Has(Command::RIGHT | Command::LEFT))
			command.SetTurn(keyHeld.Has(Command::RIGHT) - keyHeld.Has(Command::LEFT));
		else if(keyHeld.Has(Command::BACK))
		{
			if(ship.Attributes().Get("reverse thrust"))
				command |= Command::BACK;
			else
				command.SetTurn(TurnBackward(ship));
		}
		
		if(keyHeld.Has(Command::FORWARD))
			command |= Command::FORWARD;
		if(keyHeld.Has(Command::PRIMARY))
		{
			int index = 0;
			for(const Hardpoint &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && !outfit->Icon())
				{
					command.SetFire(index);
					hasGuns |= !weapon.IsTurret();
				}
				++index;
			}
		}
		if(keyHeld.Has(Command::SECONDARY))
		{
			int index = 0;
			for(const Hardpoint &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && outfit == player.SelectedWeapon())
					command.SetFire(index);
				++index;
			}
		}
		if(keyHeld.Has(Command::AFTERBURNER))
			command |= Command::AFTERBURNER;
		
		if(keyHeld.Has(AutopilotCancelKeys()))
			keyStuck = keyHeld;
	}
	if(hasGuns && Preferences::Has("Automatic aiming") && !command.Turn()
			&& ship.GetTargetShip() && ship.GetTargetShip()->GetSystem() == ship.GetSystem()
			&& ship.GetTargetShip()->IsTargetable()
			&& !keyStuck.Has(Command::LAND | Command::JUMP | Command::BOARD))
	{
		Point distance = ship.GetTargetShip()->Position() - ship.Position();
		if(distance.Unit().Dot(ship.Facing().Unit()) >= .8)
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
	}
	
	if(keyStuck.Has(Command::JUMP) && !player.HasTravelPlan())
	{
		// The player completed their travel plan, which may have indicated a destination within the final system
		keyStuck.Clear(Command::JUMP);
		const Planet *planet = player.TravelDestination();
		if(planet && planet->IsInSystem(ship.GetSystem()))
		{
			Messages::Add("Autopilot: landing on " + planet->Name() + ".");
			keyStuck |= Command::LAND;
			ship.SetTargetStellar(ship.GetSystem()->FindStellar(planet));
		}
	}
	
	// Clear "stuck" keys if actions can't be performed.
	if(keyStuck.Has(Command::LAND) && !ship.GetTargetStellar())
		keyStuck.Clear(Command::LAND);
	if(keyStuck.Has(Command::JUMP) && !(ship.GetTargetSystem() || isWormhole))
		keyStuck.Clear(Command::JUMP);
	if(keyStuck.Has(Command::BOARD) && !ship.GetTargetShip())
		keyStuck.Clear(Command::BOARD);
	
	if(ship.IsBoarding())
		keyStuck.Clear();
	else if(keyStuck.Has(Command::LAND) || (keyStuck.Has(Command::JUMP) && isWormhole))
	{
		if(ship.GetPlanet())
			keyStuck.Clear();
		else
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
	}
	else if(keyStuck.Has(Command::JUMP))
	{
		if(!ship.Attributes().Get("hyperdrive") && !ship.Attributes().Get("jump drive"))
		{
			Messages::Add("You do not have a hyperdrive installed.");
			keyStuck.Clear();
			Audio::Play(Audio::Get("fail"));
		}
		else if(!ship.JumpFuel(ship.GetTargetSystem()))
		{
			Messages::Add("You cannot jump to the selected system.");
			keyStuck.Clear();
			Audio::Play(Audio::Get("fail"));
		}
		else if(!ship.JumpsRemaining() && !ship.IsEnteringHyperspace())
		{
			Messages::Add("You do not have enough fuel to make a hyperspace jump.");
			keyStuck.Clear();
			if(keyDown.Has(Command::JUMP) || !keyHeld.Has(Command::JUMP))
				Audio::Play(Audio::Get("fail"));
		}
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			if(keyHeld.Has(Command::JUMP))
				command |= Command::WAIT;
		}
	}
	else if(keyStuck.Has(Command::BOARD))
	{
		shared_ptr<const Ship> target = ship.GetTargetShip();
		if(!CanBoard(ship, *target))
			keyStuck.Clear(Command::BOARD);
		else
		{
			MoveTo(ship, command, target->Position(), target->Velocity(), 40., .8);
			command |= Command::BOARD;
		}
	}
	
	if(isLaunching)
		command |= Command::DEPLOY;
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
			if(ship->GetSystem() == player.GetSystem() && !ship->IsDisabled())
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
	
	// Now, go through all the given ships and set their orders to the new
	// orders. But, if it turns out that they already had the given orders,
	// their orders will be cleared instead. The only command that does not
	// toggle is a move command; it always counts as a new command.
	bool hasMismatch = isMoveOrder;
	bool gaveOrder = false;
	for(const Ship *ship : ships)
	{
		// Never issue orders to a ship to target itself.
		if(ship == newTarget)
			continue;
		
		gaveOrder = true;
		hasMismatch |= !orders.count(ship);
		
		Orders &existing = orders[ship];
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
	}
	if(!gaveOrder)
		return;
	if(hasMismatch)
		Messages::Add(who + description);
	else
	{
		// Clear all the orders for these ships.
		Messages::Add(who + "no longer " + description);
		for(const Ship *ship : ships)
			orders.erase(ship);
	}
}
