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
		const string &subKey = child.Token(0);
		if(child.Size() < 2)
			child.PrintTrace("Warning: Expected belt sub-key to have a value:");
		else if(subKey == "max eccentricity")
			maxEccentricity = child.Value(1);
		else if(subKey == "scale factor closest periapsis")
			scaleFactorMinLow = child.Value(1);
		else if(subKey == "scale factor closest apoapsis")
			scaleFactorMinHigh = child.Value(1);
		else if(subKey == "scale factor farthest periapsis")
			scaleFactorMaxLow = child.Value(1);
		else if(subKey == "scale factor farthest apoapsis")
			scaleFactorMaxHigh = child.Value(1);
		else
			child.PrintTrace("Warning: Unrecognized belt sub-key:");
	}

	if(maxEccentricity < 0. || maxEccentricity > 1.)
		node.PrintTrace("Error: \"max eccentricity\" must be in the range 0-1:");
	if(scaleFactorMinLow < 0. || scaleFactorMinLow > 1.)
		node.PrintTrace("Error: \"scale factor closest periapsis\" must be in the range 0 to 1:");
	if(scaleFactorMinHigh < scaleFactorMinLow || scaleFactorMinHigh > 1.)
		node.PrintTrace("Error: \"scale factor closest apoapsis\" must be in the range"
						" \"scale factor closest periapsis\" to 1:");
	if(scaleFactorMaxLow < 1.)
		node.PrintTrace("Error: \"scale factor farthest periapsis\" must be >= 1:");
	if(scaleFactorMaxHigh < scaleFactorMaxLow)
		node.PrintTrace("Error: \"scale factor farthest apoapsis\" must be >= \"scale factor farthest periapsis\":");
}
