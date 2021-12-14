/* Sold.cpp
Copyright (c) 2021 by Hurleveur

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Sold.h"

#include <map>

namespace 
{
	const std::map<Sold::SellType, const std::string> show{{Sold::SellType::VISIBLE, ""},
		{Sold::SellType::IMPORT, "import"}, {Sold::SellType::HIDDEN, "hidden"}, {Sold::SellType::NONE, ""}};
}



double Sold::GetCost() const
{
	return cost;
}



void Sold::SetCost(double newCost)
{
	cost = newCost;
}



Sold::SellType Sold::GetSellType() const
{
	return shown;
}



const std::string &Sold::GetShown() const
{
	return show.find(shown)->second;
}



void Sold::SetBase(double cost, const Sold::SellType shown) 
{
	this->cost = cost;
	this->shown = shown;
}



Sold::SellType Sold::StringToSellType(std::string name)
{
	for(const auto& it : show)
		if(name == it.second)
			return it.first;
	return SellType::NONE;
}