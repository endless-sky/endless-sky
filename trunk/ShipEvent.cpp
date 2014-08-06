/* ShipEvent.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipEvent.h"

using namespace std;



ShipEvent::ShipEvent(shared_ptr<Ship> &actor, shared_ptr<Ship> &target, Type type)
	: actor(actor), target(target), type(type)
{
}



const shared_ptr<Ship> &ShipEvent::Actor() const
{
	return actor;
}



const shared_ptr<Ship> &ShipEvent::Target() const
{
	return target;
}



ShipEvent::Type ShipEvent::Action() const
{
	return type;
}
