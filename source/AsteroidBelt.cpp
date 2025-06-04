/* AsteroidBelt.cpp
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

#include "AsteroidBelt.h"

#include "DataNode.h"

using namespace std;



AsteroidBelt::AsteroidBelt(double radius, const DataNode &node)
	: radius(radius)
{
	Load(node);
}



void AsteroidBelt::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 1)
			continue;
		const string &key = child.Token(0);
		if(child.Size() < 2)
			child.PrintTrace("Warning: Expected belt sub-key to have a value:");
		else if(key == "max eccentricity")
			maxEccentricity = child.Value(1);
		else if(key == "scale factor closest periapsis")
			scaleFactorClosestPeriapsis = child.Value(1);
		else if(key == "scale factor closest apoapsis")
			scaleFactorClosestApoapsis = child.Value(1);
		else if(key == "scale factor farthest periapsis")
			scaleFactorFarthestPeriapsis = child.Value(1);
		else if(key == "scale factor farthest apoapsis")
			scaleFactorFarthestApoapsis = child.Value(1);
		else
			child.PrintTrace("Warning: Unrecognized belt sub-key:");
	}

	if(maxEccentricity < 0. || maxEccentricity > 1.)
		node.PrintTrace("Error: \"max eccentricity\" must be in the range [0, 1]:");
	if(scaleFactorClosestPeriapsis < 0. || scaleFactorClosestPeriapsis > 1.)
		node.PrintTrace("Error: \"scale factor closest periapsis\" must be in the range [0, 1]:");
	if(scaleFactorClosestApoapsis < scaleFactorClosestPeriapsis || scaleFactorClosestApoapsis > 1.)
		node.PrintTrace("Error: \"scale factor closest apoapsis\" must be in the range"
						" [\"scale factor closest periapsis\", 1]:");
	if(scaleFactorFarthestPeriapsis < 1.)
		node.PrintTrace("Error: \"scale factor farthest periapsis\" must be >= 1:");
	if(scaleFactorFarthestApoapsis < scaleFactorFarthestPeriapsis)
		node.PrintTrace("Error: \"scale factor farthest apoapsis\" must be >= \"scale factor farthest periapsis\":");
}
