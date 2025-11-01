/* AsteroidBelt.h
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

class DataNode;



/// Class defining the geometry of one asteroid belt, including radius and eccentricity parameters.
class AsteroidBelt {
public:
	AsteroidBelt(double radius) : radius(radius) {}
	AsteroidBelt(double radius, const DataNode &node);

	double Radius() const;
	double MaxEccentricity() const;
	double ScaleFactorClosestPeriapsis() const;
	double ScaleFactorClosestApoapsis() const;
	double ScaleFactorFarthestPeriapsis() const;
	double ScaleFactorFarthestApoapsis() const;


private:
	double radius;
	double maxEccentricity = .6;
	double scaleFactorClosestPeriapsis = .4;
	double scaleFactorClosestApoapsis = .8;
	double scaleFactorFarthestPeriapsis = 1.3;
	double scaleFactorFarthestApoapsis = 4;

	void Load(const DataNode &node);
};



/// Average radius for this belt, also used as lookup key for "remove".
inline double AsteroidBelt::Radius() const { return radius; }
/// Maximum eccentricity (default 0.6).
inline double AsteroidBelt::MaxEccentricity() const { return maxEccentricity; }
/// Factor determining periapsis closest distance relative to radius at high eccentricities (default 0.4).
inline double AsteroidBelt::ScaleFactorClosestPeriapsis() const { return scaleFactorClosestPeriapsis; }
/// Factor determining apoapsis closest distance relative to radius at low eccentricities (default 0.8).
inline double AsteroidBelt::ScaleFactorClosestApoapsis() const { return scaleFactorClosestApoapsis; }
/// Factor determining periapsis farthest distance relative to radius at low eccentricities (default 1.3).
inline double AsteroidBelt::ScaleFactorFarthestPeriapsis() const { return scaleFactorFarthestPeriapsis; }
/// Factor determining apoapsis farthest distance relative to radius at high eccentricities (default 4).
inline double AsteroidBelt::ScaleFactorFarthestApoapsis() const { return scaleFactorFarthestApoapsis; }
