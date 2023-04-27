/* ShipAICache.h
Copyright (c) 2022 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SHIP_AI_CACHE_H_
#define SHIP_AI_CACHE_H_

class Ship;



// A class which caches information needed for AI calculations of an individual ship,
// be those calculations that are needed multiple times a frame or which might only
// be needed once per frame but don't typically change from frame to frame.
class ShipAICache {
public:
	ShipAICache() = default;

	void Calibrate(const Ship &ship);
	// Get the new mass of the ship, if it changed update the weapon cache.
	void Recalibrate(const Ship &ship);

	// Accessors for AI data.
	bool IsArtilleryAI() const;
	double ShortestRange() const;
	double ShortestArtillery() const;
	double MinSafeDistance() const;


private:
	double mass = 0.;

	bool useArtilleryAI = false;
	double shortestRange = 1000.;
	double shortestArtillery = 4000.;
	double minSafeDistance = 0.;
	double maxTurningRadius = 200.;
};



// Inline the accessors and setters because they get called so frequently.
inline bool ShipAICache::IsArtilleryAI() const { return useArtilleryAI; }
inline double ShipAICache::ShortestRange() const { return shortestRange; }
inline double ShipAICache::ShortestArtillery() const { return shortestArtillery; }
inline double ShipAICache::MinSafeDistance() const { return minSafeDistance; }



#endif
