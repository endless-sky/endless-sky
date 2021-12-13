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



// Container used for the price and selling type of different items.
class Sold {
public:
	enum class SellType {
		NONE = 0,
		VISIBLE = (1 << 0),
		IMPORT = (1 << 1),
		HIDDEN = (1 << 2)
	};
	
	
public:
	double GetCost() const;
	
	void SetCost(double newCost);
	
	SellType GetSellType() const;
	
	const std::string &GetShown() const;
	
	void SetBase(double cost = 0., Sold::SellType = SellType::NONE);

	static SellType StringToSellType(std::string name);
	
	
private:
	double cost = 0.;
	SellType shown = SellType::VISIBLE;
};

#endif
