/* Legality.cpp
Copyright (c) 2020 by Eloraam

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Legality.h"

#include "GameData.h"
#include "Government.h"

using namespace std;

Legality::Legality(const DataNode& node)
{
	Load(node);
}

void Legality::Load(const DataNode& node)
{
	if(node.Size() < 2)
	{
		node.PrintTrace("No name specified for legality:");
		return;
	}
	name=node.Token(1);

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "default")
		{
			if(child.Token(1) == "atrocity")
				defaultFine=-1;
			else if(child.Token(1) == "illegal" && child.Size() >=3)
				defaultFine=child.Value(2);
		}
		else if(child.Token(0) == "atrocity")
			for(const DataNode& grand : child)
			{
				if(grand.Token(0) == "government" && grand.Size()>=2)
				{
					const Government *gov = GameData::Governments().Get(grand.Token(1));
					specificFines.emplace(gov,-1);
				}
				else
					node.PrintTrace("Skipping unrecognized attribute:");

			}
		else if(child.Token(0) == "illegal" && child.Size() >=2)
		{
			int64_t fine = child.Value(1);
			for(const DataNode& grand : child)
			{
				if(grand.Token(0) == "government" && grand.Size()>=2)
				{
					const Government *gov = GameData::Governments().Get(grand.Token(1));
					specificFines.emplace(gov,fine);
				}
				else
					node.PrintTrace("Skipping unrecognized attribute:");
			}

		}
		else
			node.PrintTrace("Skipping unrecognized attribute:");
	}
}

void Legality::SetNumeric(const string &value)
{
	name=value;
	defaultFine=DataNode::Value(value);
}

const string& Legality::Name() const
{
	return name;
}

int64_t Legality::GetFine(const Government *gov) const
{
	if(!gov) return defaultFine;
	auto it = specificFines.find(gov);
	if(it == specificFines.end())
		return defaultFine;
	return it->second;
}

