/* ShipAICache.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIPAICACHE_H_
#define SHIPAICACHE_H_

class Ship;



// Class representing the AI data of a single ship. With a seperate instance
// for every ship.
class ShipAICache {

public:
	
	ShipAICache() = default;
	// Construct and Load() at the same time.
	ShipAICache(const Ship &ship);
	
	void UpdateWeaponCache();
	
	// Accessors and setters for AI data
	bool ArtilleryAI() const;
	double ShortestRange() const;
	double ShortestArtillery() const;
	double MinSafeDistance() const;
	double TurningRadius() const;


private:
	const Ship* ship;

	bool artilleryAI = false;
	double shortestRange = 1000.;
	double shortestArtillery = 4000.;
	double minSafeDistance = 0.;
	double turningRadius = 200.;
};



// Inline the accessors and setters because they get called so frequently.
inline bool ShipAICache::ArtilleryAI() const { return artilleryAI; }
inline double ShipAICache::ShortestRange() const { return shortestRange; }
inline double ShipAICache::ShortestArtillery() const { return shortestArtillery; }
inline double ShipAICache::MinSafeDistance() const { return minSafeDistance; }
inline double ShipAICache::TurningRadius() const { return turningRadius; }



#endif
