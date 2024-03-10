/* BayType.h
Copyright (c) 2024 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BAY_TYPE_H_
#define BAY_TYPE_H_

#include <set>
#include <string>

class DataNode;



// A BayType is a list of ship categories that can be stored in a
// bay point on a Ship that shares the name of this BayType.
class BayType {
public:
	BayType() = default;
	// Construct and Load() at the same time.
	BayType(const DataNode &node);

	void Load(const DataNode &node);
	// Confirm that all categories of ship that this bay can hold
	// are carriable. If not, set isValid to false.
	void FinishLoading();

	bool Contains(const std::string &category) const;
	const std::set<std::string> &Categories() const;

	bool IsLoaded() const;
	bool IsValid() const;


private:
	std::string name;
	std::set<std::string> categories;
	bool isValid = true;
};



#endif
