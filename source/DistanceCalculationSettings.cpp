/* DistanceCalculationSettings.cpp
Copyright (c) 2023 by warp-core

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DistanceCalculationSettings.h"

#include "DataNode.h"

using namespace std;



DistanceCalculationSettings::DistanceCalculationSettings(const DataNode &node)
{
	Load(node);
}



bool DistanceCalculationSettings::operator!=(const DistanceCalculationSettings &other) const
{
	if(wormholeStrategy != other.wormholeStrategy)
		return true;
	return assumesJumpDrive != other.assumesJumpDrive;
}



void DistanceCalculationSettings::Load(const DataNode &node)
{
	for(const auto &child : node)
	{
		const string &key = child.Token(0);
		if(key == "no wormholes")
			wormholeStrategy = WormholeStrategy::NONE;
		else if(key == "only unrestricted wormholes")
			wormholeStrategy = WormholeStrategy::ONLY_UNRESTRICTED;
		else if(key == "all wormholes")
			wormholeStrategy = WormholeStrategy::ALL;
		else if(key == "assumes jump drive")
			assumesJumpDrive = true;
		else
			child.PrintTrace("Invalid distance calculation setting:");
	}
}



WormholeStrategy DistanceCalculationSettings::WormholeStrat() const
{
	return wormholeStrategy;
}



bool DistanceCalculationSettings::AssumesJumpDrive() const
{
	return assumesJumpDrive;
}
