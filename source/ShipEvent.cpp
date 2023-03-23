/* ShipEvent.cpp
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

#include "ShipEvent.h"

#include "Ship.h"

using namespace std;



ShipEvent::ShipEvent(const Government *actor, const shared_ptr<Ship> &target, int type)
	: actorGovernment(actor), target(target), type(type)
{
	if(target)
		targetGovernment = target->GetGovernment();
}



ShipEvent::ShipEvent(const shared_ptr<Ship> &actor, const shared_ptr<Ship> &target, int type)
	: actor(actor), target(target), type(type)
{
	if(actor)
		actorGovernment = actor->GetGovernment();
	if(target)
		targetGovernment = target->GetGovernment();
}



const shared_ptr<Ship> &ShipEvent::Actor() const
{
	return actor;
}



const Government *ShipEvent::ActorGovernment() const
{
	return actorGovernment;
}



const shared_ptr<Ship> &ShipEvent::Target() const
{
	return target;
}



const Government *ShipEvent::TargetGovernment() const
{
	return targetGovernment;
}




int ShipEvent::Type() const
{
	return type;
}
