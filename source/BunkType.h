/* InfoPanelState.h
 Copyright (c) 2022 by RisingLeaf(https://www.github.com/RisingLeaf)

 Endless Sky is free software: you can redistribute it and/or modify it under the
 terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later version.

 Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>


// This class is responsible for holding a BunkType.
// BunkTypes can hold passengers or crew or both or
// none.
class BunkType {
public:
	BunkType() = default;
	BunkType(std::string name, bool crew, bool passenger);

	// Function that returns the name of the bunk type.
	std::string Name() const;
	// Function that tells if this bunk type is usable by crew.
	bool CanHoldCrew() const;
	// Function that tells if this bunk type is usable by passengers.
	bool CanHoldPassengers() const;
	

private:
	std::string typeName;

	bool canHoldCrew = false;
	bool canHoldPassengers = false;
};
