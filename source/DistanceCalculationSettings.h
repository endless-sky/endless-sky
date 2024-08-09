/* DistanceCalculationSettings.h
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

#pragma once

#include "WormholeStrategy.h"

class DataNode;



class DistanceCalculationSettings {
public:
	DistanceCalculationSettings() = default;
	explicit DistanceCalculationSettings(const DataNode &node);

	bool operator!=(const DistanceCalculationSettings &other) const;

	void Load(const DataNode &node);

	WormholeStrategy WormholeStrat() const;
	bool AssumesJumpDrive() const;


private:
	enum WormholeStrategy wormholeStrategy = WormholeStrategy::NONE;
	bool assumesJumpDrive = false;
};
