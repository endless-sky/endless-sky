/* Phrase.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Phrase.h"

#include "DataNode.h"
#include "GameData.h"
#include "Random.h"

using namespace std;



void Phrase::Load(const DataNode &node)
{
	if(node.Token(0) != "phrase")
		return;
	
	parts.emplace_back();
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "word")
		{
			parts.back().emplace_back();
			Part &part = parts.back().back();
			part.phrase = nullptr;
			for(const DataNode &grand : child)
				part.words.push_back(grand.Token(0));
		}
		else if(child.Token(0) == "phrase")
		{
			parts.back().emplace_back();
			Part &part = parts.back().back();
			part.phrase = GameData::Phrases().Get(child.Token(1));
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



string Phrase::Get(int recursion_limit) const
{
	string result;
	if(parts.empty())
		return result;
	
	for(const Part &part : parts[Random::Int(parts.size())])
	{
		if(part.phrase && (recursion_limit > 0))
			result += part.phrase->Get(recursion_limit - 1);
		else if(!part.words.empty())
			result += part.words[Random::Int(part.words.size())];
	}
	
	return result;
}
