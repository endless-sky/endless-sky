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
#include "Command.h"
#include "DistanceMap.h"
#include "Government.h"
#include "Mask.h"
#include "Messages.h"
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

#include <limits>
#include <cmath>

using namespace std;

namespace {
	const Command &AutopilotCancelKeys()
	{
		static const Command keys(Command::LAND | Command::JUMP | Command::BOARD
			| Command::BACK | Command::FORWARD | Command::LEFT | Command::RIGHT);
		
		return keys;
	}
	
	bool IsStranded(const Ship &ship)
	{
		return ship.GetSystem() && !ship.GetSystem()->IsInhabited()
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
	
	static const double MAX_DISTANCE_FROM_CENTER = 10000.;
}



AI::AI()
	: step(0), keyDown(0), keyHeld(0), keyStuck(0), isLaunching(false),
	isCloaking(false), shift(false), holdPosition(false), moveToMe(false), landKeyInterval(0)
{
}



void AI::UpdateKeys(PlayerInfo &player, bool isActive)
{
	shift = (SDL_GetModState() & KMOD_SHIFT);
	
	Command oldHeld = keyHeld;
	keyHeld.ReadKeyboard();
	keyDown = keyHeld.AndNot(oldHeld);
	if(keyHeld.Has(AutopilotCancelKeys()))
		keyStuck.Clear();
	if(keyStuck.Has(Command::JUMP) && !player.HasTravelPlan())
		keyStuck.Clear(Command::JUMP);
	
	const Ship *flagship = player.Flagship();
	if(!isActive || !flagship || flagship->IsDestroyed())
		return;
	
	++landKeyInterval;
	if(oldHeld.Has(Command::LAND))
		landKeyInterval = 0;
	
	// Only toggle the "cloak" command if one of your ships has a cloaking device.
	if(keyDown.Has(Command::CLOAK))
		for(const auto &it : player.Ships())
			if(it->Attributes().Get("cloak"))
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
	if(keyDown.Has(Command::FIGHT) && target && !target->IsYours())
	{
		sharedTarget = target;
		holdPosition = false;
		moveToMe = false;
		Messages::Add("All your ships are focusing their fire on \"" + target->Name() + "\".");
	}
	if(keyDown.Has(Command::HOLD))
	{
		sharedTarget.reset();
		holdPosition = !holdPosition;
		moveToMe = false;
		Messages::Add(holdPosition ? "Your fleet is holding position."
			: "Your fleet is no longer holding position.");
	}
	if(keyDown.Has(Command::GATHER))
	{
		sharedTarget.reset();
		holdPosition = false;
		moveToMe = !moveToMe;
		Messages::Add(moveToMe ? "Your fleet is gathering around your flagship."
			: "Your fleet is no longer gathering around your flagship.");
	}
	if(sharedTarget.lock() && sharedTarget.lock()->IsDisabled())
		sharedTarget.reset();
}



void AI::UpdateEvents(const list<ShipEvent> &events)
{
	for(const ShipEvent &event : events)
	{
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(event.TargetGovernment()->IsPlayer())
				Messages::Add("You are being scanned by the " +
					event.ActorGovernment()->GetName() + " ship \"" + event.Actor()->Name() + ".\"");
		}
		if(event.Actor() && event.Target())
			actions[event.Actor()][event.Target()] |= event.Type();
		if(event.ActorGovernment()->IsPlayer() && event.Target())
		{
			int &bitmap = playerActions[event.Target()];
			int newActions = event.Type() - (event.Type() & bitmap);
			bitmap |= event.Type();
			event.TargetGovernment()->Offend(newActions, event.Target()->RequiredCrew());
		}
	}
}



void AI::Clean()
{
	actions.clear();
}



void AI::Step(const list<shared_ptr<Ship>> &ships, const PlayerInfo &player)
{
	const Ship *flagship = player.Flagship();
	
	step = (step + 1) & 31;
	int targetTurn = 0;
	for(const auto &it : ships)
	{
		// Skip any carried fighters or drones that are somehow in the list.
		if(!it->GetSystem())
			continue;
		
		if(it.get() == flagship)
		{
			MovePlayer(*it, player, ships);
			continue;
		}
		
		bool isPresent = (it->GetSystem() == player.GetSystem());
		bool isStranded = IsStranded(*it);
		if(isStranded || it->IsDisabled())
		{
			if(it->IsDestroyed() || (it->IsDisabled() && it->IsYours()) || it->GetPersonality().IsDerelict())
				continue;
			
			bool hasEnemy = false;
			Ship *firstAlly = nullptr;
			bool selectNext = false;
			Ship *nextAlly = nullptr;
			const Government *gov = it->GetGovernment();
			for(const auto &ship : ships)
			{
				if(ship->IsDisabled() || !ship->IsTargetable() || ship->GetSystem() != it->GetSystem())
					continue;
				
				const Government *otherGov = ship->GetGovernment();
				// If any enemies of this ship are in system, it cannot call for help.
				if(otherGov->IsEnemy(gov) && isPresent)
				{
					hasEnemy = true;
					break;
				}
				if((otherGov->IsPlayer() && !gov->IsPlayer()) || ship.get() == flagship)
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
				continue;
		}
		// Special case: if the player's flagship tries to board a ship to
		// refuel it, that escort should hold position for boarding.
		isStranded |= (flagship && it == flagship->GetTargetShip() && CanBoard(*flagship, *it)
			&& keyStuck.Has(Command::BOARD));
		
		Command command;
		if(it->IsYours())
		{
			if(isLaunching)
				command |= Command::DEPLOY;
			if(isCloaking)
				command |= Command::CLOAK;
		}
		
		const Personality &personality = it->GetPersonality();
		shared_ptr<Ship> parent = it->GetParent();
		
		if(isPresent && personality.IsSurveillance())
		{
			DoSurveillance(*it, command, ships);
			it->SetCommands(command);
			continue;
		}
		
		// Fire any weapons that will hit the target. Only ships that are in
		// the current system can fire.
		shared_ptr<const Ship> target = it->GetTargetShip();
		if(isPresent)
		{
			command |= AutoFire(*it, ships);
			
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			if(targetTurn == step || !target || !target->IsTargetable()
					|| (target->IsDisabled() && personality.Disables()))
				it->SetTargetShip(FindTarget(*it, ships));
		}
		
		double targetDistance = numeric_limits<double>::infinity();
		target = it->GetTargetShip();
		if(target)
			targetDistance = target->Position().Distance(it->Position());
		
		// Handle fighters:
		const string &category = it->Attributes().Category();
		bool isDrone = (category == "Drone");
		bool isFighter = (category == "Fighter");
		if(isDrone || isFighter)
		{
			bool hasSpace = true;
			hasSpace &= parent && (!isDrone || parent->DroneBaysFree());
			hasSpace &= parent && (!isFighter || parent->FighterBaysFree());
			if(!hasSpace || parent->IsDestroyed() || parent->GetSystem() != it->GetSystem())
			{
				// Handle orphaned fighters and drones.
				for(const auto &other : ships)
					if(other->GetGovernment() == it->GetGovernment() && !other->IsDisabled()
							&& other->GetSystem() == it->GetSystem())
						if((isDrone && other->DroneBaysFree()) || (isFighter && other->FighterBaysFree()))
						{
							it->SetParent(other);
							break;
						}
			}
			else if(parent && !(it->IsYours() ? isLaunching : parent->Commands().Has(Command::DEPLOY)))
			{
				it->SetTargetShip(parent);
				MoveTo(*it, command, parent->Position(), 40., .8);
				command |= Command::BOARD;
				it->SetCommands(command);
				continue;
			}
		}
		bool mustRecall = false;
		if(it->HasBays() && !(it->IsYours() ? isLaunching : it->Commands().Has(Command::DEPLOY)) && !target)
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
					|| (!shipToAssist->IsDisabled() && shipToAssist->JumpsRemaining()))
				it->SetShipToAssist(shared_ptr<Ship>());
			else if(!it->IsBoarding())
			{
				MoveTo(*it, command, shipToAssist->Position(), 40., .8);
				command |= Command::BOARD;
			}
			it->SetCommands(command);
			continue;
		}
		
		bool isPlayerEscort = it->IsYours();
		if((isPlayerEscort && holdPosition) || mustRecall || isStranded)
		{
			if(it->Velocity().Length() > .2 || !target)
				Stop(*it, command);
			else
				command.SetTurn(TurnToward(*it, TargetAim(*it)));
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
			if(personality.IsStaying())
				MoveIndependent(*it, command);
			else
				MoveEscort(*it, command);
		}
		// From here down, we're only dealing with ships that have a "parent"
		// which is in the same system as them. If you're an enemy of your
		// "parent," you don't take orders from them.
		else if(personality.IsStaying() || parent->GetGovernment()->IsEnemy(it->GetGovernment()))
			MoveIndependent(*it, command);
		// This is a friendly escort. If the parent is getting ready to
		// jump, always follow.
		else if(parent->Commands().Has(Command::JUMP))
			MoveEscort(*it, command);
		// If the player is ordering escorts to gather, don't go off to fight.
		else if(isPlayerEscort && moveToMe)
			MoveEscort(*it, command);
		// On the other hand, if the player ordered you to attack, do so even
		// if you're usually more timid than that.
		else if(isPlayerEscort && sharedTarget.lock())
			MoveIndependent(*it, command);
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
			DoCloak(*it, command, ships);
		
		// Force ships that are overlapping each other to "scatter":
		DoScatter(*it, command, ships);
		
		it->SetCommands(command);
	}
}



// Pick a new target for the given ship.
shared_ptr<Ship> AI::FindTarget(const Ship &ship, const list<shared_ptr<Ship>> &ships) const
{
	// If this ship has no government, it has no enemies.
	shared_ptr<Ship> target;
	const Government *gov = ship.GetGovernment();
	if(!gov || ship.GetPersonality().IsPacifist())
		return target;
	
	bool isPlayerEscort = ship.IsYours();
	if(isPlayerEscort)
	{
		shared_ptr<Ship> locked = sharedTarget.lock();
		if(locked && locked->GetSystem() == ship.GetSystem() && !locked->IsDisabled())
			return locked;
	}
	
	// If this ship is not armed, do not make it fight.
	double minRange = numeric_limits<double>::infinity();
	double maxRange = 0.;
	for(const Armament::Weapon &weapon : ship.Weapons())
		if(weapon.GetOutfit() && !weapon.IsAntiMissile())
		{
			minRange = min(minRange, weapon.GetOutfit()->Range());
			maxRange = max(maxRange, weapon.GetOutfit()->Range());
		}
	if(!maxRange)
		return target;
	
	shared_ptr<Ship> oldTarget = ship.GetTargetShip();
	if(oldTarget && !oldTarget->IsTargetable())
		oldTarget.reset();
	shared_ptr<Ship> parentTarget;
	if(ship.GetParent())
		parentTarget = ship.GetParent()->GetTargetShip();
	if(parentTarget && !parentTarget->IsTargetable())
		parentTarget.reset();

	// Find the closest enemy ship (if there is one). If this ship is "heroic,"
	// it will attack any ship in system. Otherwise, if all its weapons have a
	// range higher than 2000, it will engage ships up to 50% beyond its range.
	// If a ship has short range weapons and is not heroic, it will engage any
	// ship that is within 3000 of it.
	const Personality &person = ship.GetPersonality();
	double closest = person.IsHeroic() ? numeric_limits<double>::infinity() :
		(minRange > 1000.) ? maxRange * 1.5 : 4000.;
	const System *system = ship.GetSystem();
	bool isDisabled = false;
	for(const auto &it : ships)
		if(it->GetSystem() == system && it->IsTargetable() && gov->IsEnemy(it->GetGovernment()))
		{
			if(person.IsNemesis() && !it->GetGovernment()->IsPlayer())
				continue;
			// Non-escort ships should not target ships outside the range where AI ships will travel.
			if(!ship.IsYours() && it->Position().Length() >= MAX_DISTANCE_FROM_CENTER)
				continue;
			
			double range = it->Position().Distance(ship.Position());
			// Preferentially focus on your previous target or your parent ship's
			// target if they are nearby.
			if(it == oldTarget || it == parentTarget)
				range -= 500.;
			
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
			// Focus on nearly dead ships.
			range += 500. * (it->Shields() + it->Hull());
			if(range < closest)
			{
				closest = range;
				target = it;
				isDisabled = it->IsDisabled();
			}
		}
	
	bool cargoScan = ship.Attributes().Get("cargo scan");
	bool outfitScan = ship.Attributes().Get("outfit scan");
	if(!target && (cargoScan || outfitScan) && !isPlayerEscort)
	{
		closest = numeric_limits<double>::infinity();
		for(const auto &it : ships)
			if(it->GetSystem() == system && it->GetGovernment() != gov && it->IsTargetable())
			{
				if((cargoScan && !Has(ship, it, ShipEvent::SCAN_CARGO))
						|| (outfitScan && !Has(ship, it, ShipEvent::SCAN_OUTFITS)))
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
	if(!isDisabled && (ship.GetPersonality().IsFleeing() ||
			(ship.Shields() + ship.Hull() < 1. && !ship.GetPersonality().IsHeroic())))
		target.reset();
	
	return target;
}



void AI::MoveIndependent(Ship &ship, Command &command) const
{
	if(!ship.IsYours() && ship.Position().Length() >= MAX_DISTANCE_FROM_CENTER)
	{
		MoveTo(ship, command, Point(), 40., .8);
		return;
	}
	shared_ptr<const Ship> target = ship.GetTargetShip();
	if(target && (ship.GetGovernment()->IsEnemy(target->GetGovernment())
			|| (ship.IsYours() && target == sharedTarget.lock())))
	{
		bool shouldBoard = ship.Cargo().Free() && ship.GetPersonality().Plunders();
		bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
		if(shouldBoard && target->IsDisabled() && !hasBoarded)
		{
			if(ship.IsBoarding())
				return;
			MoveTo(ship, command, target->Position(), 40., .8);
			command |= Command::BOARD;
		}
		else
			Attack(ship, command, *target);
		return;
	}
	else if(target)
	{
		bool cargoScan = ship.Attributes().Get("cargo scan");
		bool outfitScan = ship.Attributes().Get("outfit scan");
		if((!cargoScan || Has(ship, target, ShipEvent::SCAN_CARGO))
				&& (!outfitScan || Has(ship, target, ShipEvent::SCAN_OUTFITS)))
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
		return;
	
	if(!ship.GetTargetSystem() && !ship.GetTargetPlanet() && !ship.GetPersonality().IsStaying())
	{
		int jumps = ship.JumpsRemaining();
		// Each destination system has an average priority of 10.
		// If you only have one jump left, landing should be high priority.
		int planetWeight = jumps ? (1 + 40 / jumps) : 1;
		
		vector<int> systemWeights;
		int totalWeight = 0;
		const vector<const System *> &links = ship.Attributes().Get("jump drive")
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
		if(!totalWeight)
			return;
		
		int choice = Random::Int(totalWeight);
		if(choice < systemTotalWeight)
		{
			for(unsigned i = 0; i < systemWeights.size(); ++i)
			{
				choice -= systemWeights[i];
				if(choice < 0)
				{
					ship.SetTargetSystem(links[i]);
					break;
				}
			}
		}
		else
		{
			choice = (choice - systemTotalWeight) / planetWeight;
			ship.SetTargetPlanet(planets[choice]);
		}
	}
	
	if(ship.GetTargetSystem())
	{
		PrepareForHyperspace(ship, command);
		bool mustWait = false;
		for(const weak_ptr<const Ship> &escort : ship.GetEscorts())
		{
			shared_ptr<const Ship> locked = escort.lock();
			mustWait = locked && locked->CanBeCarried();
		}
		
		if(!mustWait)
			command |= Command::JUMP;
	}
	else if(ship.GetTargetPlanet())
	{
		MoveToPlanet(ship, command);
		if(!ship.GetPersonality().IsStaying())
			command |= Command::LAND;
		else if(ship.Position().Distance(ship.GetTargetPlanet()->Position()) < 100.)
			ship.SetTargetPlanet(nullptr);
	}
	else if(ship.GetPersonality().IsStaying() && ship.GetSystem()->Objects().size())
	{
		unsigned i = Random::Int(ship.GetSystem()->Objects().size());
		ship.SetTargetPlanet(&ship.GetSystem()->Objects()[i]);
	}
}



void AI::MoveEscort(Ship &ship, Command &command)
{
	const Ship &parent = *ship.GetParent();
	bool isStaying = ship.GetPersonality().IsStaying();
	// If an escort is out of fuel, they should refuel without waiting for the
	// "parent" to land (because the parent may not be planning on landing).
	if(ship.Attributes().Get("fuel capacity") && !ship.JumpsRemaining() && ship.GetSystem()->IsInhabited())
		Refuel(ship, command);
	else if(ship.GetSystem() != parent.GetSystem() && !isStaying)
	{
		DistanceMap distance(ship, parent.GetSystem());
		const System *system = distance.Route(ship.GetSystem());
		ship.SetTargetSystem(system);
		if(!system || (ship.GetSystem()->IsInhabited() && !system->IsInhabited() && ship.JumpsRemaining() == 1))
			Refuel(ship, command);
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
		}
	}
	else if(parent.Commands().Has(Command::LAND) && parent.GetTargetPlanet())
	{
		ship.SetTargetPlanet(parent.GetTargetPlanet());
		MoveToPlanet(ship, command);
		if(parent.IsLanding() || parent.CanLand())
			command |= Command::LAND;
	}
	else if(parent.Commands().Has(Command::BOARD) && parent.GetTargetShip().get() == &ship)
		Stop(ship, command);
	else if(parent.Commands().Has(Command::JUMP) && parent.GetTargetSystem() && !isStaying)
	{
		DistanceMap distance(ship, parent.GetTargetSystem());
		const System *dest = distance.Route(ship.GetSystem());
		ship.SetTargetSystem(dest);
		if(!dest || (dest != parent.GetTargetSystem() && !dest->IsInhabited() && ship.JumpsRemaining() == 1))
			Refuel(ship, command);
		else
		{
			PrepareForHyperspace(ship, command);
			if(parent.IsEnteringHyperspace() || parent.CheckHyperspace())
				command |= Command::JUMP;
		}
	}
	else
		CircleAround(ship, command, parent);
}



void AI::Refuel(Ship &ship, Command &command)
{
	if(ship.GetParent() && ship.GetParent()->GetTargetPlanet()
			&& ship.GetParent()->GetTargetPlanet()->GetPlanet()->HasSpaceport())
		ship.SetTargetPlanet(ship.GetParent()->GetTargetPlanet());
	else if(!ship.GetTargetPlanet())
	{
		double closest = numeric_limits<double>::infinity();
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < closest)
				{
					ship.SetTargetPlanet(&object);
					closest = distance;
				}
			}
	}
	if(ship.GetTargetPlanet())
	{
		MoveToPlanet(ship, command);
		command |= Command::LAND;
	}
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
		double angle = asin(cross / vector.Length()) * TO_DEG;
		if(fabs(angle) <= ship.TurnRate())
			return -angle / ship.TurnRate();
	}
	
	bool left = cross < 0.;
	return left - !left;
}



bool AI::MoveToPlanet(Ship &ship, Command &command)
{
	if(!ship.GetTargetPlanet())
		return false;
	
	const Point &target = ship.GetTargetPlanet()->Position();
	return MoveTo(ship, command, target, ship.GetTargetPlanet()->Radius(), 1.);
}



bool AI::MoveTo(Ship &ship, Command &command, const Point &target, double radius, double slow)
{
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	Point distance = target - position;
	
	double speed = velocity.Length();
	
	bool isClose = (distance.Length() < radius);
	if(isClose && speed < slow)
		return true;
	
	bool shouldReverse = false;
	distance = target - StoppingPoint(ship, shouldReverse);
	bool isFacing = (distance.Unit().Dot(angle.Unit()) > .8);
	if(!isClose || (!isFacing && !shouldReverse))
		command.SetTurn(TurnToward(ship, distance));
	if(isFacing)
		command |= Command::FORWARD;
	else if(shouldReverse)
		command |= Command::BACK;
	
	return false;
}



bool AI::Stop(Ship &ship, Command &command, double slow)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	
	double speed = velocity.Length();
	
	if(speed <= slow)
		return true;
	
	if(ship.Attributes().Get("reverse thrust"))
	{
		// Figure out your stopping time using your main engine:
		double degreesToTurn = TO_DEG * acos(-velocity.Unit().Dot(angle.Unit()));
		double forwardTime = degreesToTurn / ship.TurnRate();
		forwardTime += speed / ship.Acceleration();
		
		// Figure out your reverse thruster stopping time:
		double reverseAcceleration = ship.Attributes().Get("reverse thrust") / ship.Mass();
		double reverseTime = (180. - degreesToTurn) / ship.TurnRate();
		reverseTime += speed / reverseAcceleration;
		
		if(reverseTime < forwardTime)
		{
			command.SetTurn(TurnToward(ship, velocity));
			if(velocity.Unit().Dot(angle.Unit()) > .8)
				command |= Command::BACK;
			return false;
		}
	}
	
	command.SetTurn(TurnBackward(ship));
	if(velocity.Unit().Dot(angle.Unit()) < -.8)
		command |= Command::FORWARD;
	
	return false;
}



void AI::PrepareForHyperspace(Ship &ship, Command &command)
{
	int type = ship.HyperspaceType();
	if(!type)
		return;
	
	Point direction = ship.GetTargetSystem()->Position() - ship.GetSystem()->Position();
	if(type == 150)
	{
		direction = direction.Unit();
		Point normal(-direction.Y(), direction.X());
		
		double deviation = ship.Velocity().Dot(normal);
		if(fabs(deviation) > ship.Attributes().Get("scram drive"))
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
				
				if(fabs(deviation) - correctionWhileTurning > ship.Attributes().Get("scram drive"))
					// Want to thrust from an even sharper angle
					direction = -deviation * normal;
			}
		}
		command.SetTurn(TurnToward(ship, direction));
	}
	// If we are moving too fast, point in the right direction.
	else if(Stop(ship, command, ship.Attributes().Get("jump speed")))
	{
		if(type != 200)
			command.SetTurn(TurnToward(ship, direction));
	}
}



void AI::CircleAround(Ship &ship, Command &command, const Ship &target)
{
	// This is not the behavior I want, but it's reasonable.
	Point direction = target.Position() - ship.Position();
	command.SetTurn(TurnToward(ship, direction));
	if(ship.Facing().Unit().Dot(direction) >= 0. && direction.Length() > 200.)
		command |= Command::FORWARD;
}



void AI::Attack(Ship &ship, Command &command, const Ship &target)
{
	Point d = target.Position() - ship.Position();
	
	// First, figure out what your shortest-range weapon is.
	double shortestRange = 4000.;
	bool isArmed = false;
	bool hasAmmo = false;
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		const Outfit *outfit = weapon.GetOutfit();
		if(outfit && !weapon.IsAntiMissile())
		{
			isArmed = true;
			if(!outfit->Ammo() || ship.OutfitCount(outfit->Ammo()))
				hasAmmo = true;
			shortestRange = min(outfit->Range(), shortestRange);
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
	if(shortestRange > 1000. && d.Length() < .5 * shortestRange)
	{
		command.SetTurn(TurnToward(ship, -d));
		if(ship.Facing().Unit().Dot(d) <= 0.)
			command |= Command::FORWARD;
		return;
	}
	
	// First of all, aim in the direction that will hit this target.
	command.SetTurn(TurnToward(ship, TargetAim(ship)));
	
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



void AI::DoSurveillance(Ship &ship, Command &command, const list<shared_ptr<Ship>> &ships) const
{
	const shared_ptr<Ship> &target = ship.GetTargetShip();
	if(target && (!target->IsTargetable() || target->GetSystem() != ship.GetSystem()))
		ship.SetTargetShip(shared_ptr<Ship>());
	if(target && ship.GetGovernment()->IsEnemy(target->GetGovernment()))
	{
		MoveIndependent(ship, command);
		command |= AutoFire(ship, ships);
		return;
	}
	
	bool cargoScan = ship.Attributes().Get("cargo scan");
	bool outfitScan = ship.Attributes().Get("outfit scan");
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
	else if(ship.GetTargetPlanet())
	{
		MoveToPlanet(ship, command);
		double distance = ship.Position().Distance(ship.GetTargetPlanet()->Position());
		if(distance < atmosphereScan && !Random::Int(100))
			ship.SetTargetPlanet(nullptr);
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
		shared_ptr<Ship> newTarget = FindTarget(ship, ships);
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
		
		if(jumpDrive)
			for(const System *link : ship.GetSystem()->Neighbors())
				targetSystems.push_back(link);
		else if(hyperdrive)
			for(const System *link : ship.GetSystem()->Links())
				targetSystems.push_back(link);
		
		unsigned total = targetShips.size() + targetPlanets.size() + targetSystems.size();
		if(!total)
			return;
		
		unsigned index = Random::Int(total);
		if(index < targetShips.size())
			ship.SetTargetShip(targetShips[index]);
		else
		{
			index -= targetShips.size();
			if(index < targetPlanets.size())
				ship.SetTargetPlanet(targetPlanets[index]);
			else
				ship.SetTargetSystem(targetSystems[index - targetPlanets.size()]);
		}
	}
}



void AI::DoCloak(Ship &ship, Command &command, const list<shared_ptr<Ship>> &ships)
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
			if(other->GetSystem() == ship.GetSystem() && other->IsTargetable() &&
					other->GetGovernment()->IsEnemy(ship.GetGovernment()))
				nearestEnemy = min(nearestEnemy,
					ship.Position().Distance(other->Position()));
		
		if(ship.Hull() + ship.Shields() < 1. && nearestEnemy < 2000.)
			command |= Command::CLOAK;
		
		// Also cloak if there are no enemies nearby and cloaking does
		// not cost you fuel.
		if(nearestEnemy == MAX_RANGE && !ship.Attributes().Get("cloaking fuel"))
			command |= Command::CLOAK;
	}
}



void AI::DoScatter(Ship &ship, Command &command, const list<shared_ptr<Ship>> &ships)
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



Point AI::StoppingPoint(const Ship &ship, bool &shouldReverse)
{
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	double acceleration = ship.Acceleration();
	double turnRate = ship.TurnRate();
	shouldReverse = false;
	
	// If I were to turn around and stop now, where would that put me?
	double v = velocity.Length();
	if(!v)
		return position;
	
	// This assumes you're facing exactly the wrong way.
	double degreesToTurn = TO_DEG * acos(-velocity.Unit().Dot(angle.Unit()));
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
	Point result;
	shared_ptr<const Ship> target = ship.GetTargetShip();
	if(!target)
		return result;
	
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		const Outfit *outfit = weapon.GetOutfit();
		if(!outfit || weapon.IsHoming() || weapon.IsTurret())
			continue;
		
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		Point p = target->Position() - start + ship.GetPersonality().Confusion();
		Point v = target->Velocity() - ship.Velocity();
		double steps = Armament::RendezvousTime(p, v, outfit->Velocity());
		if(!(steps == steps))
			continue;
		
		steps = min(steps, outfit->TotalLifetime());
		p += steps * v;
		
		double damage = outfit->ShieldDamage() + outfit->HullDamage();
		result += p.Unit() * damage;
	}
	
	if(!result)
		return target->Position() - ship.Position();
	return result;
}



// Fire whichever of the given ship's weapons can hit a hostile target.
Command AI::AutoFire(const Ship &ship, const list<shared_ptr<Ship>> &ships, bool secondary) const
{
	Command command;
	if(ship.GetPersonality().IsPacifist())
		return command;
	int index = -1;
	
	// Special case: your target is not your enemy. Do not fire, because you do
	// not want to risk damaging that target. The only time a ship other than
	// the player will target a friendly ship is if the player has asked a ship
	// for assistance.
	shared_ptr<Ship> currentTarget = ship.GetTargetShip();
	const Government *gov = ship.GetGovernment();
	bool isSharingTarget = ship.IsYours() && currentTarget == sharedTarget.lock();
	bool currentIsEnemy = currentTarget
		&& currentTarget->GetGovernment()->IsEnemy(gov)
		&& currentTarget->GetSystem() == ship.GetSystem();
	if(currentTarget && !(currentIsEnemy || isSharingTarget))
		currentTarget.reset();
	
	// Only fire on disabled targets if you don't want to plunder them.
	bool spareDisabled = (ship.GetPersonality().Disables() || ship.GetPersonality().Plunders());
	
	// Find the longest range of any of your non-homing weapons.
	double maxRange = 0.;
	for(const Armament::Weapon &weapon : ship.Weapons())
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
	
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		++index;
		// Skip weapons that are not ready to fire. Also skip homing weapons if
		// no target is selected, and secondary weapons if only firing primaries.
		if(!weapon.IsReady() || (!currentTarget && weapon.IsHoming()))
			continue;
		if(!secondary && weapon.GetOutfit()->Icon())
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
		double vp = outfit->Velocity();
		double lifetime = outfit->TotalLifetime();
		
		if(currentTarget && (weapon.IsHoming() || weapon.IsTurret()))
		{
			bool hasBoarded = Has(ship, currentTarget, ShipEvent::BOARD);
			if(currentTarget->IsDisabled() && spareDisabled && !hasBoarded)
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
			if(target->IsDisabled() && spareDisabled && !hasBoarded)
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
			
			const Mask &mask = target->GetSprite().GetMask(step);
			if(mask.Collide(-p, v, target->Facing()) < 1.)
			{
				command.SetFire(index);
				break;
			}
		}
	}
	
	return command;
}



void AI::MovePlayer(Ship &ship, const PlayerInfo &player, const list<shared_ptr<Ship>> &ships)
{
	Command command;
	
	if(player.HasTravelPlan())
	{
		const System *system = player.TravelPlan().back();
		ship.SetTargetSystem(system);
		// Check if there's a particular planet there we want to visit.
		for(const Mission &mission : player.Missions())
			if(mission.Destination() && mission.Destination()->GetSystem() == system)
			{
				ship.SetDestination(mission.Destination());
				break;
			}
	}
	
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
		if(!target || !CanBoard(ship, *target))
		{
			double closest = numeric_limits<double>::infinity();
			bool foundEnemy = false;
			bool foundAnything = false;
			for(const shared_ptr<Ship> &other : ships)
				if(CanBoard(ship, *other))
				{
					bool isEnemy = other->GetGovernment()->IsEnemy(ship.GetGovernment());
					double d = other->Position().Distance(ship.Position());
					if((isEnemy && !foundEnemy) || d < closest)
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
			if(!object.GetPlanet() && !object.GetSprite().IsEmpty())
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < object.Radius())
					message = object.LandingMessage();
			}
		const StellarObject *target = ship.GetTargetPlanet();
		if(target && ship.Position().Distance(target->Position()) < target->Radius())
		{
			// Special case: if there are two planets in system and you have one
			// selected, then press "land" again, do not toggle to the other if
			// you are within landing range of the one you have selected.
		}
		else if(message.empty() && target && landKeyInterval < 60)
		{
			bool found = false;
			const StellarObject *next = nullptr;
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.GetPlanet())
				{
					if(found)
					{
						next = &object;
						break;
					}
					else if(&object == ship.GetTargetPlanet())
						found = true;
				}
			if(!next)
			{
				for(const StellarObject &object : ship.GetSystem()->Objects())
					if(object.GetPlanet())
					{
						next = &object;
						break;
					}
			}
			ship.SetTargetPlanet(next);
			if(next->GetPlanet() && !next->GetPlanet()->CanLand())
				message = "The authorities on this planet refuse to clear you to land here.";
		}
		else if(message.empty())
		{
			double closest = numeric_limits<double>::infinity();
			int count = 0;
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.GetPlanet())
				{
					++count;
					double distance = ship.Position().Distance(object.Position());
					const Planet *planet = object.GetPlanet();
					if(planet == ship.GetDestination())
						distance = 0.;
					else if(!planet->HasSpaceport() && !planet->IsWormhole())
						distance += 10000.;
					
					if(distance < closest)
					{
						ship.SetTargetPlanet(&object);
						closest = distance;
					}
				}
			if(!ship.GetTargetPlanet())
				message = "There are no planets in this system that you can land on.";
			else if(!ship.GetTargetPlanet()->GetPlanet()->CanLand())
				message = "The authorities on this planet refuse to clear you to land here.";
			else if(count > 1)
			{
				message = "You can land on more than one planet in this system. Landing on ";
				if(ship.GetTargetPlanet()->Name().empty())
					message += "???.";
				else
					message += ship.GetTargetPlanet()->Name() + ".";
			}
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
	}
	else if(keyDown.Has(Command::SCAN))
		command |= Command::SCAN;
	
	bool hasGuns = Preferences::Has("Automatic firing") && !ship.IsBoarding()
		&& !(keyStuck | keyHeld).Has(Command::LAND | Command::JUMP | Command::BOARD);
	if(hasGuns)
		command |= AutoFire(ship, ships, false);
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
			for(const Armament::Weapon &weapon : ship.Weapons())
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
			for(const Armament::Weapon &weapon : ship.Weapons())
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
			&& !keyStuck.Has(Command::LAND | Command::JUMP | Command::BOARD))
	{
		Point distance = ship.GetTargetShip()->Position() - ship.Position();
		if(distance.Unit().Dot(ship.Facing().Unit()) >= .8)
			command.SetTurn(TurnToward(ship, TargetAim(ship)));
	}
	
	if(ship.IsBoarding())
		keyStuck.Clear();
	else if(keyStuck.Has(Command::LAND) && ship.GetTargetPlanet())
	{
		if(ship.GetPlanet())
			keyStuck.Clear();
		else
		{
			MoveToPlanet(ship, command);
			command |= Command::LAND;
		}
	}
	else if(keyStuck.Has(Command::JUMP) && ship.GetTargetSystem())
	{
		if(!ship.Attributes().Get("hyperdrive") && !ship.Attributes().Get("jump drive"))
		{
			Messages::Add("You do not have a hyperdrive installed.");
			keyStuck.Clear();
		}
		else if(!ship.HyperspaceType())
		{
			Messages::Add("You cannot jump to the selected system.");
			keyStuck.Clear();
		}
		else if(!ship.JumpsRemaining() && !ship.IsEnteringHyperspace())
		{
			Messages::Add("You do not have enough fuel to make a hyperspace jump.");
			keyStuck.Clear();
		}
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
			if(keyHeld.Has(Command::JUMP))
				command |= Command::WAIT;
		}
	}
	else if(keyStuck.Has(Command::BOARD) && ship.GetTargetShip())
	{
		shared_ptr<const Ship> target = ship.GetTargetShip();
		if(!CanBoard(ship, *target))
			keyStuck.Clear(Command::BOARD);
		else
		{
			MoveTo(ship, command, target->Position(), 40., .8);
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
