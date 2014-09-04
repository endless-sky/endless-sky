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
#include "GameData.h"
#include "Government.h"
#include "Key.h"
#include "Mask.h"
#include "Messages.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"

#include <SDL2/SDL.h>

#include <limits>
#include <cmath>

using namespace std;



AI::AI()
	: step(0), keyDown(0), keyHeld(0), keyStuck(0), isLaunching(false)
{
}



void AI::UpdateKeys(int keys, PlayerInfo *info, bool isActive)
{
	keyDown = keys & ~keyHeld;
	keyHeld = keys;
	if((keys & AutopilotCancelKeys()) || !isActive)
		keyStuck = 0;
	
	if(!isActive)
		return;
	
	if(keyDown & Key::Bit(Key::SELECT))
		info->SelectNext();
	if(keyDown & Key::Bit(Key::DEPLOY))
	{
		const Ship *player = info->GetShip();
		if(player && player->IsTargetable())
		{
			isLaunching = !isLaunching;
			Messages::Add(isLaunching ? "Deploying fighters" : "Recalling fighters.");
		}
	}
}



void AI::UpdateEvents(const std::list<ShipEvent> &events)
{
	for(const ShipEvent &event : events)
	{
		if(event.Type() & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		{
			if(event.TargetGovernment() == GameData::PlayerGovernment())
				Messages::Add("\"" + event.Target()->Name() + "\" is being scanned by the " +
					event.ActorGovernment()->GetName() + " ship \"" + event.Actor()->Name() + ".\"");
		}
		if(event.Actor() && event.Target())
			actions[event.Actor()][event.Target()] |= event.Type();
		if(event.ActorGovernment() == GameData::PlayerGovernment() && event.Target())
			GameData::GetPolitics().Offend(
				event.TargetGovernment(), event.Type(), event.Target()->RequiredCrew());
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
			it->ResetCommands();
			const Personality &personality = it->GetPersonality();
			shared_ptr<const Ship> parent = it->GetParent().lock();
			
			// Fire any weapons that will hit the target.
			it->SetFireCommands(AutoFire(*it, ships));
			
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			shared_ptr<const Ship> target = it->GetTargetShip().lock();
			if(targetTurn == step || !target || !target->IsTargetable()
					|| (target->IsDisabled() && personality.Disables()))
				it->SetTargetShip(FindTarget(*it, ships));
			
			double targetDistance = numeric_limits<double>::infinity();
			target = it->GetTargetShip().lock();
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
				if(parent && !parent->HasLaunchCommand())
				{
					MoveTo(*it, *it, parent->Position(), 40., .8);
					(*it).SetBoardCommand();
					continue;
				}
			}
			
			if(parent && (parent->HasLandCommand() || parent->HasHyperspaceCommand()
					|| targetDistance > 1000. || personality.IsTimid() || !target))
				MoveEscort(*it, *it);
			else
				MoveIndependent(*it, *it);
		}
	}
}



// Pick a new target for the given ship.
weak_ptr<const Ship> AI::FindTarget(const Ship &ship, const list<shared_ptr<Ship>> &ships)
{
	// If this ship has no government, it has no enemies.
	weak_ptr<const Ship> target;
	const Government *gov = ship.GetGovernment();
	if(!gov)
		return target;
	
	// If this ship is not armed, do not make it fight.
	bool isArmed = false;
	for(const Armament::Weapon &weapon : ship.Weapons())
		isArmed |= (weapon.GetOutfit() != nullptr);
	if(!isArmed)
		return target;
	
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
			if(ship.GetPersonality().IsTimid() && it->GetTargetShip().lock().get() != &ship)
				continue;
			
			double range = it->Position().Distance(ship.Position());
			if(!person.Plunders())
				range += 5000. * it->IsDisabled();
			else
			{
				bool hasBoarded = Has(ship, it, ShipEvent::BOARD);
				range += 2000. * hasBoarded;
			}
			if(range < closest)
			{
				closest = range;
				target = it;
				isDisabled = it->IsDisabled();
			}
		}
	
	bool cargoScan = ship.Attributes().Get("cargo scan");
	bool outfitScan = ship.Attributes().Get("outfit scan");
	if(!target.lock() && (cargoScan || outfitScan))
	{
		closest = numeric_limits<double>::infinity();
		for(const auto &it : ships)
			if(it->GetSystem() == system && it->GetGovernment() != ship.GetGovernment())
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
	if(!isDisabled && ship.Shields() + ship.Hull() < 1.)
		target.reset();
	
	return target;
}



void AI::MoveIndependent(Controllable &control, const Ship &ship)
{
	auto target = ship.GetTargetShip().lock();
	if(target && ship.GetGovernment()->IsEnemy(target->GetGovernment()))
	{
		bool shouldBoard = ship.Cargo().Free() && ship.GetPersonality().Plunders();
		bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
		if(shouldBoard && target->IsDisabled() && !hasBoarded)
		{
			if(ship.IsBoarding())
				return;
			MoveTo(control, ship, target->Position(), 40., .8);
			control.SetBoardCommand();
		}
		else
			Attack(control, ship, *target);
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
			CircleAround(control, ship, *target);
			control.SetScanCommand();
		}
		return;
	}
	
	if(!ship.GetTargetSystem() && !ship.GetTargetPlanet())
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
			if(object.GetPlanet() && object.GetPlanet()->HasSpaceport())
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
					control.SetTargetSystem(links[i]);
					break;
				}
			}
		}
		else
		{
			choice = (choice - systemTotalWeight) / planetWeight;
			control.SetTargetPlanet(planets[choice]);
		}
	}
	
	if(ship.GetTargetSystem())
	{
		PrepareForHyperspace(control, ship);
		bool mustWait = false;
		for(const weak_ptr<const Ship> &escort : ship.GetEscorts())
		{
			shared_ptr<const Ship> locked = escort.lock();
			mustWait = locked && locked->IsFighter();
		}
		
		if(!mustWait)
			control.SetHyperspaceCommand();
	}
	else if(ship.GetTargetPlanet())
	{
		MoveToPlanet(control, ship);
		control.SetLandCommand();
	}
}



void AI::MoveEscort(Controllable &control, const Ship &ship)
{
	const Ship &parent = *ship.GetParent().lock();
	if(ship.GetSystem() != parent.GetSystem())
	{
		control.SetTargetSystem(parent.GetSystem());
		PrepareForHyperspace(control, ship);
		control.SetHyperspaceCommand();
	}
	else if(parent.HasLandCommand() && parent.GetTargetPlanet())
	{
		control.SetTargetPlanet(parent.GetTargetPlanet());
		MoveToPlanet(control, ship);
		if(parent.IsLanding() || parent.CanLand())
			control.SetLandCommand();
	}
	else if(parent.HasBoardCommand() && parent.GetTargetShip().lock().get() == &ship)
		Stop(control, ship);
	else if(parent.HasHyperspaceCommand() && parent.GetTargetSystem())
	{
		control.SetTargetSystem(parent.GetTargetSystem());
		PrepareForHyperspace(control, ship);
		if(parent.IsHyperspacing() || parent.CanHyperspace())
			control.SetHyperspaceCommand();
	}
	else
		CircleAround(control, ship, parent);
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
	Angle angle = ship.Facing();
	bool left = vector.Cross(angle.Unit()) < 0.;
	double turn = left - !left;
	
	// Check if the ship will still be pointing to the same side of the target
	// angle if it turns by this amount.
	angle += ship.TurnRate() * turn;
	bool stillLeft = vector.Cross(angle.Unit()) < 0.;
	if(left == stillLeft)
		return turn;
	
	// If we're within one step of the correct direction, stop turning.
	return 0.;
}



bool AI::MoveToPlanet(Controllable &control, const Ship &ship)
{
	if(!ship.GetTargetPlanet())
		return false;
	
	const Point &target = ship.GetTargetPlanet()->Position();
	return MoveTo(control, ship, target, ship.GetTargetPlanet()->Radius(), 1.);
}



bool AI::MoveTo(Controllable &control, const Ship &ship, const Point &target, double radius, double slow)
{
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	Point distance = target - position;
	
	double speed = velocity.Length();
	
	if(distance.Length() < radius && speed < slow)
		return true;

	// If I am currently headed away from the planet, the first step is to
	// head towards it.
	if(distance.Dot(velocity) < 0.)
	{
		bool left = distance.Cross(angle.Unit()) < 0.;
		control.SetTurnCommand(left - !left);
	
		if(distance.Dot(angle.Unit()) > 0.)
			control.SetThrustCommand(1.);
	}
	else
	{
		distance = target - StoppingPoint(ship);
		
		// Stop steering if we're going to make it to the planet fine.
		if(distance.Length() > 20.)
		{
			bool left = distance.Cross(angle.Unit()) < 0.;
			control.SetTurnCommand(left - !left);
		}
	
		if(distance.Unit().Dot(angle.Unit()) > .8)
			control.SetThrustCommand(1.);
	}
	return false;
}



bool AI::Stop(Controllable &control, const Ship &ship, double slow)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	
	double speed = velocity.Length();
	
	if(speed <= slow)
		return true;
	
	control.SetTurnCommand(TurnBackward(ship));
	control.SetThrustCommand(velocity.Unit().Dot(angle.Unit()) < -.8);
	return false;
}



void AI::PrepareForHyperspace(Controllable &control, const Ship &ship)
{
	// If we are moving too fast, point in the right direction.
	if(Stop(control, ship, ship.Attributes().Get("jump speed")))
	{
		Point direction = ship.GetTargetSystem()->Position()
			- ship.GetSystem()->Position();
		control.SetTurnCommand(TurnToward(ship, direction));
	}
}



void AI::CircleAround(Controllable &control, const Ship &ship, const Ship &target)
{
	// This is not the behavior I want, but it's reasonable.
	Point direction = target.Position() - ship.Position();
	control.SetTurnCommand(TurnToward(ship, direction));
	control.SetThrustCommand(ship.Facing().Unit().Dot(direction) >= 0. && direction.Length() > 200.);
}



void AI::Attack(Controllable &control, const Ship &ship, const Ship &target)
{
	// First of all, aim in the direction that will hit this target.
	control.SetTurnCommand(TurnToward(ship, TargetAim(ship)));
	control.SetLaunchCommand();
	
	// This is not the behavior I want, but it's reasonable.
	Point d = target.Position() - ship.Position();
	control.SetThrustCommand(ship.Facing().Unit().Dot(d) >= 0. && d.Length() > 200.);
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
	double degreesToTurn = (180. / M_PI) * acos(-velocity.Unit().Dot(angle.Unit()));
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
Point AI::TargetAim(const Ship &ship) const
{
	Point result;
	shared_ptr<const Ship> target = ship.GetTargetShip().lock();
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
		double steps = Armament::RendevousTime(p, v, outfit->WeaponGet("velocity"));
		if(!(steps == steps))
			continue;
		
		steps = min(steps, outfit->WeaponGet("lifetime"));
		p += steps * v;
		
		double damage = outfit->WeaponGet("shield damage") + outfit->WeaponGet("hull damage");
		result += p.Unit() * damage;
	}
	
	if(!result)
		return target->Position() - ship.Position();
	return result;
}



// Fire whichever of the given ship's weapons can hit a hostile target.
int AI::AutoFire(const Ship &ship, const list<std::shared_ptr<Ship>> &ships)
{
	int bit = 1;
	int bits = 0;
	
	// Only fire on disabled targets if you don't want to plunder them.
	bool spareDisabled = (ship.GetPersonality().Disables() || ship.GetPersonality().Plunders());
	
	const Government *gov = ship.GetGovernment();
	for(const Armament::Weapon &weapon : ship.Weapons())
	{
		if(!weapon.IsReady())
		{
			bit <<= 1;
			continue;
		}
		
		const Outfit *outfit = weapon.GetOutfit();
		double vp = outfit->WeaponGet("velocity");
		double lifetime = outfit->WeaponGet("lifetime");
		
		for(auto target : ships)
		{
			if(!target->IsTargetable() || !gov->IsEnemy(target->GetGovernment())
					|| target->Velocity().Length() > 20. || (weapon.IsTurret() && target != ship.GetTargetShip().lock()))
				continue;
			
			// Don't shoot ships we want to plunder.
			bool hasBoarded = Has(ship, target, ShipEvent::BOARD);
			if(target->IsDisabled() && spareDisabled && !hasBoarded)
				continue;
			
			Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
			Point p = target->Position() - start + ship.GetPersonality().Confusion();
			Point v = target->Velocity() - ship.Velocity();
			// By the time this action is performed, the ships will have moved
			// forward one time step.
			p += v;
			
			if(weapon.IsHoming() || weapon.IsTurret())
			{
				double steps = Armament::RendevousTime(p, v, vp);
				if(steps == steps && steps <= lifetime)
				{
					bits |= bit;
					break;
				}
			}
			else
			{
				// Get the vector the weapon will travel along.
				v = (ship.Facing() + weapon.GetAngle()).Unit() * vp - v;
				// Extrapolate over the lifetime of the projectile.
				v *= lifetime;
				
				const Mask &mask = target->GetSprite().GetMask(step);
				if(mask.Collide(-p, v, target->Facing()) < 1.)
				{
					bits |= bit;
					break;
				}
			}
		}
		bit <<= 1;
	}
	
	return bits;
}



void AI::MovePlayer(Controllable &control, const PlayerInfo &info, const list<shared_ptr<Ship>> &ships)
{
	const Ship &ship = *info.GetShip();
	control.ResetCommands();
	
	if(info.HasTravelPlan())
	{
		const System *system = info.TravelPlan().back();
		control.SetTargetSystem(system);
		// Check if there's a particular planet there we want to visit.
		for(const Mission &mission : info.Missions())
			if(mission.Destination() && mission.Destination()->GetSystem() == system)
			{
				control.SetDestination(mission.Destination());
				break;
			}
		for(const Mission *mission : info.SpecialMissions())
			if(mission->Destination() && mission->Destination()->GetSystem() == system)
			{
				control.SetDestination(mission->Destination());
				break;
			}
	}
	
	if(keyDown & Key::Bit(Key::NEAREST))
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
				if(!state)
					continue;
				
				state += state * !other->IsDisabled();
				
				double d = other->Position().Distance(ship.Position());
				
				if(state > closeState || (state == closeState && d < closest))
				{
					control.SetTargetShip(other);
					closest = d;
					closeState = state;
				}
			}
	}
	else if(keyDown & Key::Bit(Key::TARGET))
	{
		const Government *playerGovernment = info.GetShip()->GetGovernment();
		bool targetMine = SDL_GetModState() & KMOD_SHIFT;
		
		shared_ptr<const Ship> target = control.GetTargetShip().lock();
		bool selectNext = !target;
		for(const shared_ptr<Ship> &other : ships)
		{
			if(other == target)
				selectNext = true;
			else if(other.get() != &ship && selectNext && other->IsTargetable() &&
					(other->GetGovernment() == playerGovernment) == targetMine)
			{
				control.SetTargetShip(other);
				selectNext = false;
				break;
			}
		}
		if(selectNext)
			control.SetTargetShip(weak_ptr<Ship>());
	}
	else if(keyDown & Key::Bit(Key::BOARD))
	{
		shared_ptr<const Ship> target = control.GetTargetShip().lock();
		if(!target || !target->IsDisabled() || target->Hull() <= 0.)
		{
			double closest = numeric_limits<double>::infinity();
			bool foundEnemy = false;
			for(const shared_ptr<Ship> &other : ships)
				if(other->IsTargetable() && other->IsDisabled() && other->Hull() > 0.)
				{
					bool isEnemy = other->GetGovernment()->IsEnemy(ship.GetGovernment());
					double d = other->Position().Distance(ship.Position());
					if((isEnemy && !foundEnemy) || d < closest)
					{
						closest = d;
						foundEnemy = isEnemy;
						control.SetTargetShip(other);
					}
				}
		}
	}
	else if(keyDown & Key::Bit(Key::LAND))
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
		if(message.empty() && control.GetTargetPlanet())
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
					else if(&object == control.GetTargetPlanet())
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
			control.SetTargetPlanet(next);
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
					if(object.GetPlanet() == ship.GetDestination())
						distance = 0.;
					
					if(distance < closest)
					{
						control.SetTargetPlanet(&object);
						closest = distance;
					}
				}
			if(count > 1)
				message = "You can land on more than one planet in this system. Landing on "
					+ control.GetTargetPlanet()->Name() + ".";
		}
		if(!message.empty())
			Messages::Add(message);
	}
	else if(keyDown & Key::Bit(Key::SCAN))
		control.SetScanCommand();
	
	if(keyHeld)
	{
		if(keyHeld & Key::Bit(Key::BACK))
			control.SetTurnCommand(TurnBackward(ship));
		else
			control.SetTurnCommand(
				((keyHeld & Key::Bit(Key::RIGHT)) != 0) -
				((keyHeld & Key::Bit(Key::LEFT)) != 0));
		
		if(keyHeld & Key::Bit(Key::FORWARD))
			control.SetThrustCommand(1.);
		if(keyHeld & Key::Bit(Key::PRIMARY))
		{
			int index = 0;
			bool hasGuns = false;
			for(const Armament::Weapon &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && !outfit->Ammo())
				{
					control.SetFireCommand(index);
					hasGuns |= !weapon.IsTurret();
				}
				++index;
			}
			if(hasGuns && !control.GetTurnCommand())
				control.SetTurnCommand(TurnToward(ship, TargetAim(ship)));
		}
		if(keyHeld & Key::Bit(Key::SECONDARY))
		{
			int index = 0;
			for(const Armament::Weapon &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && outfit == info.SelectedWeapon())
					control.SetFireCommand(index);
				++index;
			}
		}
		
		if(keyHeld & AutopilotCancelKeys())
			keyStuck = keyHeld;
	}
	
	if(ship.IsBoarding())
		keyStuck = 0;
	else if((keyStuck & Key::Bit(Key::LAND)) && ship.GetTargetPlanet())
	{
		if(ship.GetPlanet())
			keyStuck = 0;
		else
		{
			MoveToPlanet(control, ship);
			control.SetLandCommand();
		}
	}
	else if((keyStuck & Key::Bit(Key::JUMP)) && ship.GetTargetSystem())
	{
		if(!ship.JumpsRemaining() && !ship.IsHyperspacing())
		{
			Messages::Add("You do not have enough fuel to make a hyperspace jump.");
			keyStuck = 0;
			return;
		}
		PrepareForHyperspace(control, ship);
		control.SetHyperspaceCommand();
	}
	else if((keyStuck & Key::Bit(Key::BOARD)) && ship.GetTargetShip().lock())
	{
		shared_ptr<const Ship> target = control.GetTargetShip().lock();
		MoveTo(control, ship, target->Position(), 40., .8);
		control.SetBoardCommand();
	}
	
	if(isLaunching)
		control.SetLaunchCommand();
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



int AI::AutopilotCancelKeys()
{
	return Key::Bit(Key::LAND) | Key::Bit(Key::JUMP) | Key::Bit(Key::BOARD) |
		Key::Bit(Key::BACK) | Key::Bit(Key::RIGHT) | Key::Bit(Key::LEFT) | Key::Bit(Key::FORWARD);
}
