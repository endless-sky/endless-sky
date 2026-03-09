/* Loadout.h
Copyright (c) 2026 by Noelle Devonshire

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

#include "Ship.h"

#include <map>
#include <string>



// This class represents a loadout. It stores the outfits data related to the
// loadout, and also handles reading and writing of files on the system.
class Loadout {
public:
	Loadout(std::string name, std::string shipModel, std::map<const Outfit*, int> &outfits);

	// Attempt to create a loadout from a given file. Will return nullptr if a loadout could not be created.
	static Loadout *Load(const std::filesystem::path &path);
	static bool Exists(const std::string & name);

	std::string ShipModel() const;
	std::map<const Outfit*, int> Outfits();
	std::string Name() const;

	bool Save() const;
	bool Delete() const;

private:
	static std::filesystem::path GetFilepath(const std::string &fileName);


private:
	const std::string name;
	const std::string shipModel;
	std::map<const Outfit*, int> outfits;
};
