/* Sold.h
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SOLD_H_
#define SOLD_H_

#include <string>



// Container used for the price and selling type of different items (outfits in this case).
class Sold {
public:
	enum ShowSold {
		NONE = 0,
		DEFAULT = (1 << 0),
		IMPORT = (1 << 1),
		HIDDEN = (1 << 2)
	};
	
	
public:
	double GetCost() const;
	
	void SetCost(double newCost);
	
	ShowSold GetShown() const;
	
	const std::string &GetShow() const;
	
	void SetBase(double cost = 0., std::string shown = "");
	
	
private:
	double cost = 0.;
	ShowSold shown = ShowSold::DEFAULT;
};

#endif
