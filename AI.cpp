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
#include "Government.h"
#include "Key.h"
#include "Mask.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Ship.h"
#include "System.h"

#include <limits>
#include <cmath>

using namespace std;



AI::AI()
	: step(0), keyDown(0), keyHeld(0), keyStuck(0)
{
}



void AI::UpdateKeys(int keys, PlayerInfo *info)
{
	keyDown = keys & ~keyHeld;
	keyHeld = keys;
	if(keys)
		keyStuck = 0;
	
	if(keyDown & Key::Bit(Key::SELECT))
		info->SelectNext();
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
			
			// Fire any weapons that will hit the target.
			it->SetFireCommands(AutoFire(*it, ships));
			
			// Each ship only switches targets twice a second, so that it can
			// focus on damaging one particular ship.
			targetTurn = (targetTurn + 1) & 31;
			shared_ptr<const Ship> target = it->GetTargetShip().lock();
			if(targetTurn == step || !target || target->IsFullyDisabled()
					|| !target->IsTargetable())
				it->SetTargetShip(FindTarget(*it, ships));
			
			double targetDistance = numeric_limits<double>::infinity();
			target = it->GetTargetShip().lock();
			if(target)
				targetDistance = target->Position().Distance(it->Position());
			
			if(it->GetParent().lock() && targetDistance > 1000.)
				MoveEscort(*it, *it);
			else
				MoveIndependent(*it, *it);
		}
	}
}



// Get any messages (such as "you cannot land here!") to display.
const string &AI::Message() const
{
	return message;
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
	double closest = numeric_limits<double>::infinity();
	const System *system = ship.GetSystem();
	for(const auto &it : ships)
		if(it->GetSystem() == system && it->IsTargetable() && gov->IsEnemy(it->GetGovernment()))
		{
			// TODO: ship personalities controlling whether they destroy disabled ships.
			if(it->IsFullyDisabled())
				continue;
			
			double range = it->Position().Distance(ship.Position());
			range += 5000. * it->IsFullyDisabled();
			if(range < closest)
			{
				closest = range;
				target = it;
			}
		}
	
	return target;
}



void AI::MoveIndependent(Controllable &control, const Ship &ship)
{
	auto target = ship.GetTargetShip().lock();
	if(target)
	{
		Attack(control, ship, *target);
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
		if(jumps)
		{
			for(const System *link : ship.GetSystem()->Links())
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
		
		int choice = rand() % totalWeight;
		if(choice < systemTotalWeight)
		{
			for(unsigned i = 0; i < systemWeights.size(); ++i)
			{
				choice -= systemWeights[i];
				if(choice < 0)
				{
					control.SetTargetSystem(ship.GetSystem()->Links()[i]);
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
	control.SetTargetShip(parent.GetTargetShip());
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



void AI::MoveToPlanet(Controllable &control, const Ship &ship)
{
	if(!ship.GetTargetPlanet())
		return;
	
	const Point &target = ship.GetTargetPlanet()->Position();
	const Point &position = ship.Position();
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	Point distance = target - position;
	
	double speed = velocity.Length();
	
	if(distance.Length() < ship.GetTargetPlanet()->Radius() && speed < 1.)
		return;
	else
	{
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
	}
}



void AI::PrepareForHyperspace(Controllable &control, const Ship &ship)
{
	const Point &velocity = ship.Velocity();
	const Angle &angle = ship.Facing();
	
	// If we are moving too fast, point in the right direction.
	double speed = velocity.Length();
	if(speed > .2)
	{
		control.SetTurnCommand(TurnBackward(ship));
		control.SetThrustCommand(velocity.Unit().Dot(angle.Unit()) < -.8);
	}
	else
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
		Point p = target->Position() - start + ship.Confusion();
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
					|| target->Velocity().Length() > 20.)
				continue;
			
			// TODO: determine from a ship's "personality" whether it will kill
			// a ship that is already disabled. Also distinguish 
			if(target->IsFullyDisabled())
				continue;
			
			Point start = ship.Position() + ship.Facing().Rotate(weapon.GetPoint());
			Point p = target->Position() - start + ship.Confusion();
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
	
	message.clear();
	
	if(info.HasTravelPlan())
		control.SetTargetSystem(info.TravelPlan().back());
	
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
				
				state += state * !other->IsFullyDisabled();
				
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
		
		shared_ptr<const Ship> target = control.GetTargetShip().lock();
		bool selectNext = !target;
		for(const shared_ptr<Ship> &other : ships)
			if(other->GetGovernment() != playerGovernment)
			{
				if(other == target)
					selectNext = true;
				else if(selectNext && other->IsTargetable())
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
		if(!target || !target->IsFullyDisabled() || target->Hull() <= 0.)
		{
			double closest = numeric_limits<double>::infinity();
			bool foundEnemy = false;
			for(const shared_ptr<Ship> &other : ships)
				if(other->IsTargetable() && other->IsFullyDisabled() && other->Hull() > 0.)
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
	
	if(keyHeld & Key::Bit(Key::LAND))
	{
		// If the player is right over an uninhabited planet, display a message
		// explaining why they cannot land there.
		for(const StellarObject &object : ship.GetSystem()->Objects())
			if(!object.GetPlanet() && !object.GetSprite().IsEmpty())
			{
				double distance = ship.Position().Distance(object.Position());
				if(distance < object.Radius())
					message = object.LandingMessage();
			}
		if(message.empty())
		{
			double closest = numeric_limits<double>::infinity();
			for(const StellarObject &object : ship.GetSystem()->Objects())
				if(object.GetPlanet())
				{
					double distance = ship.Position().Distance(object.Position());
					if(distance < closest)
					{
						control.SetTargetPlanet(&object);
						closest = distance;
					}
				}
		}
	}
	
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
			for(const Armament::Weapon &weapon : ship.Weapons())
			{
				const Outfit *outfit = weapon.GetOutfit();
				if(outfit && !outfit->Ammo())
					control.SetFireCommand(index);
				++index;
			}
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
		
		keyStuck = keyHeld;
	}
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
		PrepareForHyperspace(control, ship);
		control.SetHyperspaceCommand();
	}
	else
		keyStuck = 0;
}
