/* Planet.h
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PORT_H_
#define PORT_H_

#include "Sale.h"

#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

class DataNode;
class Fleet;
class Government;
class Outfit;
class PlayerInfo;
class Ship;
class Sprite;
class System;



// Class representing a port on a planet.
class Port
{
public:
	// The different properties that can be recharged by a port.
	class RechargeType
	{
	public:
		static constexpr int None = 0;
		static constexpr int Shields = (1 << 0);
		static constexpr int Hull = (1 << 1);
		static constexpr int Energy = (1 << 2);
		static constexpr int Fuel = (1 << 3);
		static constexpr int All = Shields | Hull | Energy | Fuel;
	};

	// The different services available on this port.
	class ServicesType
	{
	public:
		static constexpr int None = 0;
		static constexpr int Trading = (1 << 0);
		static constexpr int JobBoard = (1 << 1);
		static constexpr int Bank = (1 << 2);
		static constexpr int HireCrew = (1 << 3);
		static constexpr int OffersMissions = (1 << 4);
		static constexpr int All = Trading | JobBoard | Bank | HireCrew | OffersMissions;
	};


public:
	// The name of this port.
	std::string name;

	// The description of this port. Shown when clicking on the
	// port button on the planet panel.
	std::string description;

	// What is recharged when landing on this port.
	int recharge = RechargeType::None;

	// What services are available on this port.
	int services = ServicesType::None;

	// Whether this port has news.
	bool hasNews = false;
};



#endif
