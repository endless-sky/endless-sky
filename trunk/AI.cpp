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
#include "GameData.h"
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
}



AI::AI()
	: step(0), keyDown(0), keyHeld(0), keyStuck(0), isLaunching(false),
	isCloaking(false), shift(false), holdPosition(false), moveToMe(false)
{
}



void AI::UpdateKeys(PlayerInfo *info, bool isActive)
{
	shift = (SDL_GetModState() & KMOD_SHIFT);
	
	Command oldHeld = keyHeld;
	keyHeld.ReadKeyboard();
	keyDown = keyHeld & ~oldHeld;
	if(keyHeld & AutopilotCancelKeys())
		keyStuck.Clear();
	if(keyStuck.Has(Command::JUMP) && !info->HasTravelPlan())
		keyStuck.Clear(Command::JUMP);
	
	const Ship *player = info->GetShip();
	if(!isActive || !player)
		return;
	
	// Cloaking device.
	if(keyDown.Has(Command::CLOAK) && player->Attributes().Get("cloak"))
	{
		isCloaking = !isCloaking;
		Messages::Add(isCloaking ? "Engaging cloaking device." : "Disengaging cloaking device.");
	}
	if(!player->IsTargetable())
		return;
	
	if(keyDown.Has(Command::SELECT))
		info->SelectNext();
	
	// The commands below here only apply if you have escorts or fighters.
	if(info->Ships().size() < 2)
		return;
	
	if(keyDown.Has(Command::DEPLOY) && player->HasBays())
	{
		isLaunching = !isLaunching;
		Messages::Add(isLaunching ? "Deploying fighters" : "Recalling fighters.");
	}
	shared_ptr<Ship> target = player->GetTargetShip();
	if(keyDown.Has(Command::FIGHT) && target)
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
}



void AI::UpdateEvents(const std::list<ShipEvent> &events)
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



void AI::Step(const list<shared_ptr<Ship>> &ships, const PlayerInfo &info)
{
	const Ship *player = info.GetShip();
	
	step = (step + 1) & 31;
	int targetTurn = 0;
	for(const auto &it : ships)
	{
		if(it.get() == player)
			MovePlayer(*it, info, ships);
		else
		{
			Command command;
			
			const Personality &personality = it->GetPersonality();
			shared_ptr<Ship> parent = it->GetParent();
			
			bool isPresent = (it->GetSystem() == info.GetSystem());
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
				if(!parent)
				{
					// Handle orphaned fighters and drones.
					for(const auto &other : ships)
						if(other->GetGovernment() == it->GetGovernment())
							if((isDrone && other->DroneBaysFree())
									|| (isFighter && other->FighterBaysFree()))
							{
								it->SetParent(other);
								break;
							}
				}
				if(parent && !parent->Commands().Has(Command::DEPLOY))
				{
					it->SetTargetShip(parent);
					MoveTo(*it, command, parent->Position(), 40., .8);
					command |= Command::BOARD;
					it->SetCommands(command);
					continue;
				}
			}
			
			shared_ptr<Ship> shipToAssist = it->GetShipToAssist();
			if(shipToAssist)
			{
				it->SetTargetShip(shipToAssist);
				if(shipToAssist->IsDestroyed() || shipToAssist->GetSystem() != it->GetSystem())
					it->SetShipToAssist(weak_ptr<Ship>());
				else if(!it->IsBoarding())
				{
					MoveTo(*it, command, shipToAssist->Position(), 40., .8);
					command |= Command::BOARD;
				}
				it->SetCommands(command);
				continue;
			}
			
			bool isPlayerEscort = it->GetGovernment()->IsPlayer();
			if(isPlayerEscort && holdPosition)
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
			else if(parent && !parent->IsDisabled()
					&& ((parent->Commands() & (Command::LAND | Command::JUMP))
						|| parent->GetSystem() != it->GetSystem()
						|| targetDistance > 2000. || personality.IsTimid() || !target
						|| (!it->JumpsRemaining() && it->Attributes().Get("fuel capacity"))
						|| (isPlayerEscort && moveToMe))
					&& (parent->GetSystem() != it->GetSystem()
						|| !parent->GetGovernment()->IsEnemy(it->GetGovernment()))
					&& (!target || personality.IsTimid() || parent->GetSystem() != it->GetSystem())
					&& !(personality.IsStaying() && parent->GetSystem() != it->GetSystem()))
				MoveEscort(*it, command);
			else
				MoveIndependent(*it, command);
			
			if(it->Attributes().Get("afterburner thrust") && target && !target->IsDisabled()
					&& target->IsTargetable() && target->GetSystem() == it->GetSystem())
			{
				double fuel = it->Fuel() * it->Attributes().Get("fuel capacity");
				if(fuel - it->Attributes().Get("afterburner fuel") >= it->Attributes().Get("jump fuel"))
					if(command.Has(Command::FORWARD) && targetDistance < 1000.)
						command |= Command::AFTERBURNER;
			}
			DoCloak(*it, command, ships);
			it->SetCommands(command);
		}
	}
}



// Pick a new target for the given ship.
weak_ptr<Ship> AI::FindTarget(const Ship &ship, const list<shared_ptr<Ship>> &ships) const
{
	// If this ship has no government, it has no enemies.
	weak_ptr<Ship> target;
	const Government *gov = ship.GetGovernment();
	if(!gov)
		return target;
	
	bool isPlayerEscort = (gov->IsPlayer());
	if(isPlayerEscort)
	{
		shared_ptr<Ship> locked = sharedTarget.lock();
		if(locked && locked->GetSystem() == ship.GetSystem() && !locked->IsDisabled())
			return sharedTarget;
	}
	
	// If this ship is not armed, do not make it fight.
	bool isArmed = false;
	for(const Armament::Weapon &weapon : ship.Weapons())
		isArmed |= (weapon.GetOutfit() != nullptr);
	if(!isArmed)
		return target;
	
	shared_ptr<Ship> oldTarget = ship.GetTargetShip();
	if(oldTarget && !oldTarget->IsTargetable())
		oldTarget.reset();
	shared_ptr<Ship> parentTarget;
	if(ship.GetParent())
		parentTarget = ship.GetParent()->GetTargetShip();
	if(parentTarget && !parentTarget->IsTargetable())
		parentTarget.reset();

	// Find the closest enemy ship (if there is one).
	const Personality &person = ship.GetPersonality();
	double closest = numeric_limits<double>::infinity();
	const System *system = ship.GetSystem();
	bool isDisabled = false;
	for(const auto &it : ships)
		if(it->GetSystem() == system && it->IsTargetable() && gov->IsEnemy(it->GetGovernment()))
		{
			// "Timid" ships do not pick fights; they only attack ships that are
			// already targeting them.
			if(ship.GetPersonality().IsTimid() && it->GetTargetShip().get() != &ship)
				continue;
			
			if(person.IsNemesis() && !it->GetGovernment()->IsPlayer())
				continue;
			
			double range = it->Position().Distance(ship.Position());
			// Preferentially focus on your previous target or your parent ship's
			// target if they are nearby.
			if(it == oldTarget || it == parentTarget)
				range -= 500.;
			
			// If your personality it to disable ships rather than destroy them,
			// never target disabled ships.
			if(it->IsDisabled() && person.Disables() && !person.Plunders())
				continue;
			
			if(!person.Plunders())
				range += 5000. * it->IsDisabled();
			else
			{
				bool hasBoarded = Has(ship, it, ShipEvent::BOARD);
				range += 2000. * hasBoarded;
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
	if(!target.lock() && (cargoScan || outfitScan) && !isPlayerEscort)
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
	if(ship.Position().Length() >= 10000.)
	{
		MoveTo(ship, command, Point(), 40., .8);
		return;
	}
	shared_ptr<const Ship> target = ship.GetTargetShip();
	if(target && ship.GetGovernment()->IsEnemy(target->GetGovernment()))
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
			mustWait = locked && locked->IsFighter();
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
		if(!system || (!system->IsInhabited() && ship.JumpsRemaining() == 1))
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
		if(dest != parent.GetTargetSystem() && !dest->IsInhabited() && ship.JumpsRemaining() == 1)
			Refuel(ship, command);
		else
		{
			PrepareForHyperspace(ship, command);
			if(parent.IsEnteringHyperspace() || parent.CanHyperspace())
				command |= Command::JUMP;
		}
	}
	else
		CircleAround(ship, command, parent);
}



void AI::Refuel(Ship &ship, Command &command)
{
	if(ship.GetParent() && ship.GetParent()->GetTargetPlanet())
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
	Angle angle = ship.Facing();
	bool left = ship.Velocity().Cross(angle.Unit()) > 0.;
	double turn = left - !left;
	
	// Check if the ship will still be pointing to the same side of the target
	// angle if it turns by this amount.
	angle += ship.TurnRate() * turn;
	bool stillLeft = ship.Velocity().Cross(angle.Unit()) > 0.;
	if(left == stillLeft)
		return turn;
	
	// If we're within one step of the correct direction, stop turning.
	return 0.;
}



double AI::TurnToward(const Ship &ship, const Point &vector)
{
	static const double RAD_TO_DEG = 180. / 3.14159265358979;
	Point facing = ship.Facing().Unit();
	double cross = vector.Cross(facing);
	
	if(vector.Dot(facing) > 0.)
	{
		double angle = asin(cross / vector.Length()) * RAD_TO_DEG;
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
	
	bool isVeryClose = (distance.Length() < .5 * radius);
	distance = target - StoppingPoint(ship);
	bool isFacing = (distance.Unit().Dot(angle.Unit()) > .8);
	if(!isVeryClose && (!isClose || !isFacing))
		command.SetTurn(TurnToward(ship, distance));
	if(isFacing || (isVeryClose && velocity.Dot(angle.Unit()) < 0.))
		command |= Command::FORWARD;
	
	return false;
}



bool AI::Stop(Ship &ship, Command &command, double slow)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	
	double speed = velocity.Length();
	
	if(speed <= slow)
		return true;
	
	command.SetTurn(TurnBackward(ship));
	if(velocity.Unit().Dot(angle.Unit()) < -.8)
		command |= Command::FORWARD;
	
	return false;
}



void AI::PrepareForHyperspace(Ship &ship, Command &command)
{
	Point direction = ship.GetTargetSystem()->Position() - ship.GetSystem()->Position();
	if(ship.Attributes().Get("scram drive"))
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
		command.SetTurn(TurnToward(ship, direction));
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
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		const Outfit *outfit = weapon.GetOutfit();
		if(!outfit || outfit->WeaponGet("anti-missile"))
			continue;
		
		shortestRange = min(outfit->Range(), shortestRange);
	}
	
	// Deploy any fighters you are carrying.
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



void AI::DoSurveillance(Ship &ship, Command &command, const std::list<std::shared_ptr<Ship>> &ships) const
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
			ship.SetTargetShip(weak_ptr<Ship>());
		else
		{
			CircleAround(ship, command, *target);
			command |= Command::SCAN;
		}
	}
	else
	{
		shared_ptr<Ship> newTarget = FindTarget(ship, ships).lock();
		if(newTarget && ship.GetGovernment()->IsEnemy(newTarget->GetGovernment()))
		{
			ship.SetTargetShip(newTarget);
			return;
		}
		
		vector<weak_ptr<Ship>> targetShips;
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



void AI::DoCloak(Ship &ship, Command &command, const std::list<std::shared_ptr<Ship>> &ships)
{
	if(ship.Attributes().Get("cloak"))
	{
		// Never cloak if it will cause you to be stranded.
		if(ship.Attributes().Get("cloaking fuel") && !ship.Attributes().Get("ramscoop"))
		{
			double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
			fuel -= ship.Attributes().Get("cloaking fuel");
			if(fuel < ship.Attributes().Get("jump fuel"))
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



Point AI::StoppingPoint(const Ship &ship)
{
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	double acceleration = ship.Acceleration();
	double turnRate = ship.TurnRate();
	
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
		if(!outfit || weapon.IsHoming() || weapon.IsTurret() || outfit->Ammo())
			continue;
		
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		Point p = target->Position() - start + ship.GetPersonality().Confusion();
		Point v = target->Velocity() - ship.Velocity();
		double steps = Armament::RendevousTime(p, v, outfit->WeaponGet("velocity"));
		if(!(steps == steps))
			continue;
		
		steps = min(steps, outfit->Lifetime());
		p += steps * v;
		
		double damage = outfit->ShieldDamage() + outfit->HullDamage();
		result += p.Unit() * damage;
	}
	
	if(!result)
		return target->Position() - ship.Position();
	return result;
}



// Fire whichever of the given ship's weapons can hit a hostile target.
Command AI::AutoFire(const Ship &ship, const list<std::shared_ptr<Ship>> &ships, bool secondary) const
{
	Command command;
	int index = -1;
	
	// Special case: your target is not your enemy. Do not fire, because you do
	// not want to risk damaging that target. The only time a ship other than
	// the player will target a friendly ship is if the player has asked a ship
	// for assistance.
	if(ship.GetTargetShip() && !ship.GetTargetShip()->GetGovernment()->IsEnemy(ship.GetGovernment()))
		return command;
	
	// Only fire on disabled targets if you don't want to plunder them.
	bool spareDisabled = (ship.GetPersonality().Disables() || ship.GetPersonality().Plunders());
	
	// Find the longest range of any of your non-homing weapons.
	double maxRange = 0.;
	for(const Armament::Weapon &weapon : ship.Weapons())
		if(weapon.IsReady() && !weapon.IsHoming() && (secondary || !weapon.GetOutfit()->Ammo()))
			maxRange = max(maxRange, weapon.GetOutfit()->Range());
	// Extend the weapon range slightly to account for velocity differences.
	maxRange *= 1.5;
	
	// Find all enemy ships within range of at least one weapon.
	const Government *gov = ship.GetGovernment();
	vector<shared_ptr<const Ship>> enemies;
	for(auto target : ships)
		if(target->IsTargetable() && gov->IsEnemy(target->GetGovernment())
				&& target->Velocity().Length() < 20.
				&& target->GetSystem() == ship.GetSystem()
				&& target->Position().Distance(ship.Position()) < maxRange)
			enemies.push_back(target);
	
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		++index;
		if(!weapon.IsReady() || (!ship.GetTargetShip() && weapon.IsHoming())
				|| (!secondary && weapon.GetOutfit()->Ammo()))
			continue;
		
		if(weapon.GetOutfit()->WeaponGet("firing fuel"))
		{
			double fuel = ship.Fuel() * ship.Attributes().Get("fuel capacity");
			fuel -= weapon.GetOutfit()->WeaponGet("firing fuel");
			// If the ship is not ever leaving this system, it does not need to
			// reserve any fuel.
			bool isStaying = ship.GetPersonality().IsStaying();
			if(!secondary || (fuel < ship.Attributes().Get("jump fuel") * !isStaying))
				continue;
		}
		Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
		start += ship.GetPersonality().Confusion();
		
		const Outfit *outfit = weapon.GetOutfit();
		double vp = outfit->WeaponGet("velocity");
		double lifetime = outfit->Lifetime();
		
		if(ship.GetTargetShip() && (weapon.IsHoming() || weapon.IsTurret()))
		{
			Point p = ship.GetTargetShip()->Position() - start;
			Point v = ship.GetTargetShip()->Velocity() - ship.Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;
			
			if(p.Length() < outfit->WeaponGet("blast radius"))
				continue;
			
			double steps = Armament::RendevousTime(p, v, vp);
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
			if(!target->IsTargetable() || !gov->IsEnemy(target->GetGovernment())
					|| target->Velocity().Length() > 20.
					|| target->GetSystem() != ship.GetSystem())
				continue;
			
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



void AI::MovePlayer(Ship &ship, const PlayerInfo &info, const list<shared_ptr<Ship>> &ships)
{
	Command command;
	
	if(info.HasTravelPlan())
	{
		const System *system = info.TravelPlan().back();
		ship.SetTargetSystem(system);
		// Check if there's a particular planet there we want to visit.
		for(const Mission &mission : info.Missions())
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
				if(!state && !shift)
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
			if(other == target)
				selectNext = true;
			else if(other.get() != &ship && selectNext && other->IsTargetable() &&
					other->GetGovernment()->IsPlayer() == shift)
			{
				ship.SetTargetShip(other);
				selectNext = false;
				break;
			}
		}
		if(selectNext)
			ship.SetTargetShip(weak_ptr<Ship>());
	}
	else if(keyDown.Has(Command::BOARD))
	{
		shared_ptr<const Ship> target = ship.GetTargetShip();
		if(!target || !target->IsDisabled() || target->IsDestroyed())
		{
			double closest = numeric_limits<double>::infinity();
			bool foundEnemy = false;
			for(const shared_ptr<Ship> &other : ships)
				if(other->IsTargetable() && other->IsDisabled() && !other->IsDestroyed())
				{
					bool isEnemy = other->GetGovernment()->IsEnemy(ship.GetGovernment());
					double d = other->Position().Distance(ship.Position());
					if((isEnemy && !foundEnemy) || d < closest)
					{
						closest = d;
						foundEnemy = isEnemy;
						ship.SetTargetShip(other);
					}
				}
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
		else if(message.empty() && target)
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
				message = "You can land on more than one planet in this system. Landing on "
					+ ship.GetTargetPlanet()->Name() + ".";
		}
		if(!message.empty())
			Messages::Add(message);
	}
	else if(keyDown.Has(Command::JUMP))
	{
		if(!ship.GetTargetSystem())
		{
			double bestMatch = -2.;
			for(const System *link : ship.GetSystem()->Links())
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
	
	bool hasGuns = Preferences::Has("Automatic firing");
	if(hasGuns)
		command |= AutoFire(ship, ships, false);
	if(keyHeld)
	{
		if(keyHeld.Has(Command::BACK))
			command.SetTurn(TurnBackward(ship));
		else
			command.SetTurn(keyHeld.Has(Command::RIGHT) - keyHeld.Has(Command::LEFT));
		
		if(keyHeld.Has(Command::FORWARD))
			command |= Command::FORWARD;
		if(keyHeld.Has(Command::PRIMARY))
		{
			int index = 0;
			for(const Armament::Weapon &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && !outfit->Ammo() && !outfit->WeaponGet("firing fuel"))
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
				if(outfit && outfit == info.SelectedWeapon())
					command.SetFire(index);
				++index;
			}
		}
		if(keyHeld.Has(Command::AFTERBURNER))
			command |= Command::AFTERBURNER;
		
		if(keyHeld & AutopilotCancelKeys())
			keyStuck = keyHeld;
	}
	if(hasGuns && Preferences::Has("Automatic aiming") && !command.Turn()
			&& ship.GetTargetShip() && ship.GetTargetShip()->GetSystem() == ship.GetSystem()
			&& !(keyStuck & (Command::LAND | Command::JUMP | Command::BOARD)))
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
		if(!ship.JumpsRemaining() && !ship.IsEnteringHyperspace())
		{
			Messages::Add("You do not have enough fuel to make a hyperspace jump.");
			keyStuck.Clear();
		}
		else
		{
			PrepareForHyperspace(ship, command);
			command |= Command::JUMP;
		}
	}
	else if(keyStuck.Has(Command::BOARD) && ship.GetTargetShip())
	{
		shared_ptr<const Ship> target = ship.GetTargetShip();
		MoveTo(ship, command, target->Position(), 40., .8);
		command |= Command::BOARD;
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
