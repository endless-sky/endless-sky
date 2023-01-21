/* Fleet.h
 Copyright (c) 2014 by RisingLeaf

 Endless Sky is free software: you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later version.

 Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FLEET_H_
#define FLEET_H_

#include "Ship.h"

#include <string>
#include <memory>



// Class to hold one instance of a fleet.
class Fleet {
public:
	Fleet() = default;
	Fleet(std::string name, std::shared_ptr<Ship> flagship, std::vector<std::shared_ptr<Ship>> ships);

	std::string Name() const;
	void SetName(std::string newName);

	std::shared_ptr<Ship> Flagship() const;
	void SetFlagship(std::shared_ptr<Ship> newFlagship);

	std::vector<std::shared_ptr<Ship>> Ships() const;
	void SetShips(std::vector<std::shared_ptr<Ship>> newShips);

	bool ShouldBeRemoved() const;

private:
	std::string name;
	std::shared_ptr<Ship> flagship;
	std::vector<std::shared_ptr<Ship>> ships;
};



#endif
