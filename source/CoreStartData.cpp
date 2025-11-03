/* CoreStartData.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "CoreStartData.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Planet.h"
#include "System.h"

using namespace std;



void CoreStartData::Load(const DataNode &node)
{
	identifier = node.Size() >= 2 ? node.Token(1) : "Unidentified Start";
	for(const DataNode &child : node)
	{
		// Check for the "add" keyword. CoreStateData currently doesn't support the "remove" keyword.
		bool add = (child.Token(0) == "add");
		if(add && child.Size() < 2)
		{
			child.PrintTrace("Skipping " + child.Token(0) + " with no key given:");
			continue;
		}
		if(!LoadChild(child, add))
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void CoreStartData::Save(DataWriter &out) const
{
	out.Write("start", identifier);
	out.BeginChild();
	{
		out.Write("system", system->TrueName());
		out.Write("planet", planet->TrueName());
		if(date)
			out.Write("date", date.Day(), date.Month(), date.Year());
		accounts.Save(out);
	}
	out.EndChild();
}



Date CoreStartData::GetDate() const
{
	return date ? date : Date(16, 11, 3013);
}



const Planet &CoreStartData::GetPlanet() const
{
	return (planet && planet->IsValid()) ? *planet : *GameData::Planets().Get("New Boston");
}



const System &CoreStartData::GetSystem() const
{
	if(system && system->IsValid())
		return *system;
	const System *planetSystem = GetPlanet().GetSystem();

	return (planetSystem && planetSystem->IsValid()) ? *planetSystem : *GameData::Systems().Get("Rutilicus");
}



const Account &CoreStartData::GetAccounts() const noexcept
{
	return accounts;
}



const string &CoreStartData::Identifier() const noexcept
{
	return identifier;
}



bool CoreStartData::LoadChild(const DataNode &child, bool isAdd)
{
	const string &key = child.Token(isAdd ? 1 : 0);
	int valueIndex = isAdd ? 2 : 1;
	bool hasValue = (child.Size() > valueIndex);
	const string &value = child.Token(hasValue ? valueIndex : 0);

	if(key == "date" && child.Size() >= valueIndex + 3)
		date = Date(child.Value(valueIndex), child.Value(valueIndex + 1), child.Value(valueIndex + 2));
	else if(key == "system" && hasValue)
		system = GameData::Systems().Get(value);
	else if(key == "planet" && hasValue)
		planet = GameData::Planets().Get(value);
	else if(key == "account")
		accounts.Load(child, !isAdd);
	else
		return false;

	return true;
}
