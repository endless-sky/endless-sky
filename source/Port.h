/* Port.h
Copyright (c) 2023 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PORT_H_
#define PORT_H_

#include "Paragraphs.h"

#include <string>

class DataNode;



// Class representing a port on a planet and its capabilities, such as what properties
// of a ship it can recharge and what services (e.g. banking, trading) it can provide.
class Port
{
public:
	// The different ship properties that can be recharged by a port.
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
	// Load a port's description from a node.
	void Load(const DataNode &node);
	void LoadDefaultSpaceport();
	void LoadUninhabitedSpaceport();

	// Load a port's description text paragraphs from the planet spaceport description.
	void LoadDescription(const DataNode &node);

	// Whether this port was loaded from the Load function.
	bool CustomLoaded() const;

	// Whether this port has any services available.
	bool HasServices() const;

	// Get all the possible sources that can get recharged at this port.
	int GetRecharges() const;

	const std::string &Name() const;
	const Paragraphs &Description() const;

	// Check whether the given recharging is possible.
	bool CanRecharge(int type) const;
	// Check whether the given service is available.
	bool HasService(int type) const;

	bool HasNews() const;


private:
	// Whether this port was loaded from the Load function.
	bool loaded = false;

	// The name of this port.
	std::string name;

	// The description of this port. Shown when clicking on the
	// port button on the planet panel.
	Paragraphs description;

	// What is recharged when landing on this port.
	int recharge = RechargeType::None;

	// What services are available on this port.
	int services = ServicesType::None;

	// Whether this port has news.
	bool hasNews = false;
};



#endif
