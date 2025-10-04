/* Flotsam.h
Copyright (c) 2016 by Michael Zahniser

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

#include "Body.h"

#include "Angle.h"

#include <string>
#include <vector>

class Effect;
class Government;
class Outfit;
class Point;
class Ship;
class Visual;



class Flotsam : public Body {
public:
	// Constructors for flotsam carrying either a commodity or an outfit.
	Flotsam(const std::string &commodity, int count, const Government *sourceGovernment = nullptr);
	Flotsam(const Outfit *outfit, int count, const Government *sourceGovernment = nullptr);

	// Functions provided by the Body base class:
	// Frame GetFrame(int step = -1) const;
	// const Point &Position() const;
	// const Point &Velocity() const;
	// const Angle &Facing() const;
	// Point Unit() const;

	// Place this flotsam, and set the given ship as its source. This is a
	// separate function because a ship may queue up flotsam to dump but take
	// several frames before it finishes dumping it all.
	void Place(const Ship &source);
	// Place this flotsam with its starting position at the specified bay of the source ship,
	// instead of the center of the ship.
	void Place(const Ship &source, size_t bayIndex);
	// Place flotsam coming from something other than a ship. Optionally specify
	// the maximum relative velocity, or the exact relative velocity as a vector.
	void Place(const Body &source, double maxVelocity = .5);
	void Place(const Body &source, const Point &dv);

	// Move the object one time-step forward.
	void Move(std::vector<Visual> &visuals);
	void SetVelocity(const Point &velocity);

	// This is the one ship that cannot pick up this flotsam.
	const Ship *Source() const;
	// Ships from this Government should not pick up this flotsam because it
	// was explicitly dumped by a member of this government. (NPCs typically
	// perform this type of dumping to appease pirates.)
	const Government *SourceGovernment() const;
	// This is what the flotsam contains:
	const std::string &CommodityType() const;
	const Outfit *OutfitType() const;
	int Count() const;
	// This is how big one "unit" of the flotsam is (in tons). If a ship has
	// less than this amount of space, it can't pick up anything here.
	double UnitSize() const;
	double Mass() const;

	// Transfer contents to the collector ship. The flotsam velocity is
	// stabilized in proportion to the amount being transferred.
	int TransferTo(Ship *collector);


public:
	// Amount of tons that is expected per box.
	static const int TONS_PER_BOX;


private:
	Angle spin;
	int lifetime = 0;
	double drag = 0.999;

	const Ship *source = nullptr;
	std::string commodity;
	const Outfit *outfit = nullptr;
	int count = 0;
	const Government *sourceGovernment = nullptr;
};
