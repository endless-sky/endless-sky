/* Galaxy.h
Copyright (c) 2014 by Michael Zahniser

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

#include "Point.h"

class DataNode;
class Sprite;



// This is any object that should be drawn as a backdrop to the map. Multiple
// galaxies can be handled by just spacing them out so widely that the player
// will never accidentally scroll the view from one to the other.
class Galaxy {
public:
	void Load(const DataNode &node);

	const Point &Position() const;
	const Sprite *GetSprite() const;


private:
	Point position;
	const Sprite *sprite = nullptr;
};
