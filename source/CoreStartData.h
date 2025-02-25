/* CoreStartData.h
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Account.h"
#include "Date.h"

#include <string>

class DataNode;
class DataWriter;
class Planet;
class System;



// Base class containing data of a starting scenario that is useful for later
// reference (e.g. determining the in-game starting date, where the player began,
// or how financially secure they were). One-time information, such as ships,
// conditions, and the conversation, are not saved. Scenario authors desiring this
// data should encode it into the applied starting conditions.
class CoreStartData {
public:
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// The planet on which the player begins (or New Boston, if not set).
	const Planet &GetPlanet() const;
	// The system in which the game begins (or Rutilicus, if not set).
	const System &GetSystem() const;
	// The date on which the game begins (or 16 Nov 3013, if not set).
	Date GetDate() const;
	// The initial credits, debts, and credit rating for the player.
	const Account &GetAccounts() const noexcept;

	// Get the internal identifier for this starting scenario.
	const std::string &Identifier() const noexcept;


protected:
	// Returns true if the child node was handled by this class.
	bool LoadChild(const DataNode &child, bool isAdd);


protected:
	// The planet on which the game begins.
	const Planet *planet = nullptr;
	// The system in which the game begins.
	const System *system = nullptr;
	// The date on which the game begins.
	Date date;
	// Initial credits, debts, and credit rating.
	Account accounts;
	// The key, if any, used to identify this start in data files.
	std::string identifier;
};
