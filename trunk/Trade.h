/* Trade.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TRADE_H_
#define TRADE_H_

#include "DataFile.h"

#include <vector>
#include <string>



// Class representing all the commodities that are available to trade.
class Trade {
public:
	class Commodity {
	public:
		std::string name;
		int low;
		int high;
		std::vector<std::string> items;
	};
	
	
public:
	void Load(const DataFile::Node &node);
	
	const std::vector<Commodity> &Commodities() const;
	
	
private:
	std::vector<Commodity> commodities;
};



#endif
