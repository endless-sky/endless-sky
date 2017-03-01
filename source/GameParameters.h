/* GameParameters.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAMEPARAMETERS_H_
#define GAMEPARAMETERS_H_

#include "DataNode.h"

#include <vector>
#include <string>



class GameParameters {
public:
	void Load(const DataNode &node);

	// These are all the possible category strings for ships.
	const std::vector<std::string> &ShipCategories() const;

	double DepreciationFull() const;
	double DepreciationDaily() const;
	int DepreciationMaxAge() const;

private:
	void LoadDepreciation(const DataNode &node);
	void LoadShipCategories(const DataNode &node);
};


#endif
