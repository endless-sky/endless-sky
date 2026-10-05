/* Industry.cpp
Copyright (c) 2026 by Endless Sky Expanded contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Industry.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Facility.h"
#include "Set.h"

#include <algorithm>

using namespace std;



void Industry::Load(const DataNode &node, const Set<Facility> &facilities)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) != "facility" || child.Size() < 2)
		{
			child.PrintTrace("Skipping unrecognized attribute:");
			continue;
		}
		const Facility *type = facilities.Find(child.Token(1));
		if(!type || !type->IsDefined())
		{
			child.PrintTrace("Skipping facility of undefined type:");
			continue;
		}

		Holding holding;
		holding.type = type;
		for(const DataNode &grand : child)
		{
			const string &key = grand.Token(0);
			if(key == "planet" && grand.Size() >= 2)
				holding.planet = grand.Token(1);
			else if(key == "stockpile" && grand.Size() >= 2)
				holding.stockpile = max(0, static_cast<int>(grand.Value(1)));
			else
				grand.PrintTrace("Skipping unrecognized attribute:");
		}
		if(holding.planet.empty())
			child.PrintTrace("Skipping facility with no planet:");
		else if(!Find(*type, holding.planet))
			holdings.push_back(std::move(holding));
	}
}



void Industry::Save(DataWriter &out) const
{
	if(holdings.empty())
		return;

	out.Write("industry");
	out.BeginChild();
	{
		for(const Holding &holding : holdings)
		{
			out.Write("facility", holding.type->TrueName());
			out.BeginChild();
			{
				out.Write("planet", holding.planet);
				if(holding.stockpile)
					out.Write("stockpile", holding.stockpile);
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



const vector<Industry::Holding> &Industry::Holdings() const
{
	return holdings;
}



Industry::Holding *Industry::Find(const Facility &type, const string &planet)
{
	for(Holding &holding : holdings)
		if(holding.type == &type && holding.planet == planet)
			return &holding;
	return nullptr;
}



const Industry::Holding *Industry::Find(const Facility &type, const string &planet) const
{
	return const_cast<Industry *>(this)->Find(type, planet);
}



void Industry::Build(const Facility &type, const string &planet)
{
	if(!Find(type, planet))
		holdings.push_back({&type, planet, 0});
}



void Industry::AdvanceDay()
{
	for(Holding &holding : holdings)
		holding.stockpile = max(holding.stockpile,
			min(holding.stockpile + holding.type->OutputPerDay(), holding.type->Storage()));
}



int Industry::Collect(Holding &holding, int space)
{
	int amount = max(0, min(holding.stockpile, space));
	holding.stockpile -= amount;
	return amount;
}
