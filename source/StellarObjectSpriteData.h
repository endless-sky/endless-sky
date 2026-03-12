/* StellarObjectSpriteData.h
Copyright (c) 2025 by Amazinite

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

#include <string>

class DataNode;
class Sprite;



// A class that stores information about how a stellar object should behave based off of its sprite.
class StellarObjectSpriteData {
public:
	StellarObjectSpriteData() = default;
	explicit StellarObjectSpriteData(const DataNode &node);
	void Load(const DataNode &node);

	void SetLandingMessage(const std::string &message);
	void SetMass(double mass);

	const std::string &LandingMessage() const;
	double SolarPower() const;
	double SolarWind() const;
	const Sprite *StarIcon() const;
	double HabitableDistance() const;
	double Mass() const;


private:
	std::string landingMessage;
	double solarPower = 0.;
	double solarWind = 0.;
	const Sprite *starIcon = nullptr;
	double habitable = 0.;
	double mass = 0.;
};
