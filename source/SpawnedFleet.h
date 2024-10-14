/* SpawnedFleet.h
Copyright (c) 2023 by an anonymous author

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SPAWNED_FLEET_H_
#define SPAWNED_FLEET_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

class Ship;



class SpawnedFleet: public std::enable_shared_from_this<SpawnedFleet>
{
public:
	SpawnedFleet() = default;
	SpawnedFleet(const std::string &category);
	SpawnedFleet(const std::string &category, std::list<std::shared_ptr<Ship>> &ships);

	// Set each ship's spawned fleet to this fleet.
	void ConnectToShips();

	std::string &Category();
	const std::string &Category() const;

	std::vector<std::weak_ptr<Ship>> &Ships();
	const std::vector<std::weak_ptr<Ship>> &Ships() const;

	size_t CountShips() const;
	size_t CountNonDisabledShips() const;

	void PruneShips();

private:
	std::string category;
	std::vector<std::weak_ptr<Ship>> ships;
};

#endif
