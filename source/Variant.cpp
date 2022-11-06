/* Variant.cpp
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Variant.h"

#include "DataNode.h"
#include "GameData.h"
#include "Ship.h"

#include <algorithm>

using namespace std;



Variant::Variant(const DataNode &node)
{
	Load(node);
}



void Variant::Load(const DataNode &node)
{
	if(node.Token(0) == "variant" && node.Size() >= 2)
		weight = max<int>(1, node.Value(1));
	else if(node.Token(0) == "add" && node.Size() >= 3)
		weight = max<int>(1, node.Value(2));

	for(const DataNode &child : node)
	{
		int n = 1;
		if(child.Size() >= 2 && child.Value(1) >= 1.)
			n = child.Value(1);
		ships.insert(ships.end(), n, GameData::Ships().Get(child.Token(0)));
	}
}



// Determine if this variant template uses well-defined data.
bool Variant::IsValid() const
{
	// At least one valid ship is enough to make the variant valid.
	if(any_of(ships.begin(), ships.end(),
			[](const Ship *const s) noexcept -> bool { return s->IsValid(); }))
		return true;

	return false;
}



int Variant::Weight() const
{
	return weight;
}



const vector<const Ship *> &Variant::Ships() const
{
	return ships;
}



// The strength of a variant is the sum of the strength of its ships.
int64_t Variant::Strength() const
{
	int64_t strength = 0;
	for(const Ship *ship : ships)
		strength += ship->Strength();
	return strength;
}



bool Variant::operator==(const Variant &other) const
{
	// Are the ships of other a permutation of this variant's?
	if(other.ships.size() != ships.size()
		|| !is_permutation(ships.begin(), ships.end(), other.ships.begin()))
		return false;

	// If all checks have passed, these variants are equal.
	return true;
}



bool Variant::operator!=(const Variant &other) const
{
	return !(*this == other);
}
