/* AI.cpp
Michael Zahniser, 27 Jan 2014

Function definitions for the AI class.
*/

#include "AI.h"

#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Ship.h"
#include "System.h"

#include <limits>
#include <cmath>

using namespace std;



void AI::UpdateKeys()
{
	keys.Update();
}



void AI::Step(const list<shared_ptr<Ship>> &ships, const PlayerInfo &info)
{
	const Ship *player = info.GetShip();
	
	for(const auto &it : ships)
	{
		if(it.get() == player)
			MovePlayer(*it, info, ships);
		else if(it->GetParent().lock())
			MoveEscort(*it, *it);
		else
			MoveIndependent(*it, *it);
	}
}



// Get any messages (such as "you cannot land here!") to display.
const string &AI::Message() const
{
	return message;
}



void AI::MoveIndependent(Controllable &control, const Ship &ship)
{
	control.ResetCommands();
	
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
	control.ResetCommands();
	
	shared_ptr<const Ship> parentPtr = ship.GetParent().lock();
	if(!parentPtr)
	{
		MoveIndependent(control, ship);
		return;
	}
	const Ship &parent = *parentPtr;
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
	if(speed > .1)
	{
		control.SetTurnCommand(TurnBackward(ship));
		control.SetThrustCommand(velocity.Unit().Dot(angle.Unit()) < -.5);
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



void AI::MovePlayer(Controllable &control, const PlayerInfo &info, const list<shared_ptr<Ship>> &ships)
{
	const Ship &ship = *info.GetShip();
	control.ResetCommands();
	
	message.clear();
	
	if(info.HasTravelPlan())
		control.SetTargetSystem(info.TravelPlan().back());
	
	if(keys.Status() & KeyStatus::TARGET_NEAR)
	{
		double closest = numeric_limits<double>::infinity();
		for(const shared_ptr<Ship> &other : ships)
			if(other.get() != &ship && other->GetSystem() == ship.GetSystem() && other->IsTargetable())
			{
				double d = other->Position().Distance(ship.Position());
				if(d < closest)
				{
					control.SetTargetShip(other);
					closest = d;
				}
			}
	}
	
	if(keys.Status() & KeyStatus::LAND)
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
	
	if(keys.Status())
	{
		if(keys.Status() & KeyStatus::BACK)
			control.SetTurnCommand(TurnBackward(ship));
		else
			control.SetTurnCommand(keys.Turn());
		
		if(keys.Status() & KeyStatus::THRUST)
			control.SetThrustCommand(keys.Thrust());
		if(keys.Status() & KeyStatus::PRIMARY)
			control.SetFireCommand(0);
		
		sticky = keys;
	}
	else if((sticky.Status() & KeyStatus::LAND) && ship.GetTargetPlanet())
	{
		if(ship.HasLanded())
			sticky.Clear();
		else
		{
			MoveToPlanet(control, ship);
			control.SetLandCommand();
		}
	}
	else if((sticky.Status() & KeyStatus::HYPERSPACE) && ship.GetTargetSystem())
	{
		PrepareForHyperspace(control, ship);
		control.SetHyperspaceCommand();
	}
	else
		sticky.Clear();
}
