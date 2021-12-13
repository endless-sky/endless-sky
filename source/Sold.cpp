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
	const std::map<Sold::ShowSold, const std::string> show{{Sold::ShowSold::DEFAULT, ""},
															{Sold::ShowSold::IMPORT, "import"},
															{Sold::ShowSold::HIDDEN, "hidden"},
															{Sold::ShowSold::NONE, ""}};
}



double Sold::GetCost() const
{
	return cost;
}



void Sold::SetCost(double newCost)
{
	cost = newCost;
}



Sold::ShowSold Sold::GetShown() const
{
	return shown;
}



const std::string &Sold::GetShow() const
{
	return show.find(shown)->second;
}



void Sold::SetBase(double cost, const std::string shown) 
{
	this->cost = cost;
	if(shown == "")
		this->shown = ShowSold::DEFAULT;
	else if(shown == "import")
		this->shown = ShowSold::IMPORT;
	else if(shown == "hidden")
		this->shown = ShowSold::HIDDEN;
}