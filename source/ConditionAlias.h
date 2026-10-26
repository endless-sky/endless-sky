/* ConditionAlias.h
Copyright (c) 2026 by Amazinite

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

#include "ConditionSet.h"

class ConditionsStore;
class DataNode;



// A condition set that can be defined by a root node and called be name in other condition sets.
// Useful for simplifying complex condition sets into a single token.
class ConditionAlias {
public:
	ConditionAlias() = default;

	void Load(const DataNode &node, const ConditionsStore *playerConditions);

	bool Test() const;


private:
	ConditionSet conditions;
};
