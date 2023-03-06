/* MaskManager.h
Copyright (c) 2021 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MASK_MANAGER_H_
#define MASK_MANAGER_H_

#include "Mask.h"

#include <map>
#include <mutex>
#include <vector>

class Mask;
class Sprite;



// Class that stores the masks for sprites that have them, and provides the correct
// mask for the scale that the sprite requests.
class MaskManager {
public:
	// Move the given masks at 1x scale into the manager's storage.
	void SetMasks(const Sprite *sprite, std::vector<Mask> &&masks);

	// Add a scale that the given sprite needs to have a mask for.
	void RegisterScale(const Sprite *sprite, double scale);

	// Create the scaled versions of all masks from the 1x versions.
	void ScaleMasks();

	// Get the masks for the given sprite at the given scale. If a
	// sprite has no masks, an empty mask is returned.
	const std::vector<Mask> &GetMasks(const Sprite *sprite, double scale) const;


private:
	std::map<const Sprite *, std::map<double, std::vector<Mask>>> spriteMasks;

	// Mutex to make sure different threads don't modify the masks at the same time.
	std::mutex spriteMutex;
};



#endif
