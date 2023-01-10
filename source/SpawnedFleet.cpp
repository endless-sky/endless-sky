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

#include "Ship.h"
#include "SpawnedFleet.h"

using namespace std;

SpawnedFleet::SpawnedFleet(const string &id)
	: id(id)
{
}



SpawnedFleet::SpawnedFleet(const string &id, list<shared_ptr<Ship>> &ships)
	: id(id), ships(ships.begin(), ships.end())
{
}



void SpawnedFleet::ConnectToShips()
{
	for(auto it = ships.begin(); it!=ships.end();)
		try {
			shared_ptr<Ship> ship(*it);
			ship->SetSpawnedFleet(shared_from_this());
			++it;
		}
		catch(const bad_weak_ptr &sadness)
		{
			it = ships.erase(it);
		}
}



string &SpawnedFleet::Id()
{
	return id;
}



const string &SpawnedFleet::Id() const
{
	return id;
}



vector<weak_ptr<Ship>> &SpawnedFleet::Ships()
{
	return ships;
}



const vector<weak_ptr<Ship>> &SpawnedFleet::Ships() const
{
	return ships;
}



size_t SpawnedFleet::CountShips() const
{
	size_t count = 0;
	for(auto &it : ships)
		if(!it.expired())
			count++;
	return count;
}



void SpawnedFleet::PruneShips()
{
	for(auto it = ships.begin(); it != ships.end();)
		try {
			shared_ptr<Ship> ship(*it);
			if(ship->IsDestroyed())
				it = ships.erase(it);
			else
				++it;
		}
		catch(const bad_weak_ptr &bwp)
		{
			printf("%s: pruned a ship with no references\n", id.c_str());
			it = ships.erase(it);
		}
}
