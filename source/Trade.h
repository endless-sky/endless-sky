/* Trade.h
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

#include <string>
#include <vector>

class DataNode;



// Class representing all the commodities that are available to trade. Each
// commodity has a certain normal price range, and can also specify specific
// items that are a kind of that commodity, so that a mission can have you
// deliver, say, "eggs" or "frozen meat" instead of generic "food".
class Trade {
public:
	class Commodity {
	public:
		std::string name;
		int low = 0;
		int high = 0;
		std::vector<std::string> items;
	};


public:
	void Load(const DataNode &node);

	const std::vector<Commodity> &Commodities() const;
	const std::vector<Commodity> &SpecialCommodities() const;


private:
	std::vector<Commodity> commodities;
	std::vector<Commodity> specialCommodities;
};
