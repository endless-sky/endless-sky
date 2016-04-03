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
	if(node.Size() > 1)
		name = node.Token(1);
	
	parts.emplace_back();
	for(const DataNode &child : node)
	{
		parts.back().emplace_back();
		Part &part = parts.back().back();
		
		if(child.Token(0) == "word")
		{
			for(const DataNode &grand : child)
				part.words.push_back(grand.Token(0));
		}
		else if(child.Token(0) == "phrase")
		{
			const Phrase *subphrase = GameData::Phrases().Get(child.Token(1));
			if(subphrase->ReferencesPhrase(name))
				child.PrintTrace("Found recursive phrase reference:");
			else
				part.phrase = subphrase;
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



string Phrase::Get() const
{
	string result;
	if(parts.empty())
		return result;
	
	for(const Part &part : parts[Random::Int(parts.size())])
	{
		if(part.phrase)
			result += part.phrase->Get();
		else if(!part.words.empty())
			result += part.words[Random::Int(part.words.size())];
	}
	
	return result;
}



bool Phrase::ReferencesPhrase(const std::string& name) const
{
	if(name == this->name)
		return true;
	
	for(const vector<Part> &alternative : parts)
		for(const Part &part : alternative)
			if(part.phrase && part.phrase->ReferencesPhrase(name))
				return true;
	
	return false;
}
