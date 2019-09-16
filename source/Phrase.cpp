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
#include "Format.h"
#include "GameData.h"
#include "Random.h"

using namespace std;

namespace {
	// Replace oldStr with newStr in target.
	string Replace(const string &target, const string &oldStr, const string &newStr)
	{
		const size_t oldSize = oldStr.size();
		const size_t newSize = newStr.size();
		string result(target);
		for(size_t pos = 0;;)
		{
			pos = result.find(oldStr, pos);
			if(pos == string::npos)
				break;
			result.replace(pos, oldSize, newStr);
			pos += newSize;
		}
		return result;
	}
}



void Phrase::Load(const DataNode &node)
{
	// Set the name of this phrase, so we know it has been loaded.
	name = node.Size() >= 2 ? node.Token(1) : "Unnamed Phrase";
	
	parts.emplace_back();
	for(const DataNode &child : node)
	{
		parts.back().emplace_back();
		Part &part = parts.back().back();
		
		if(child.Token(0) == "word")
		{
			for(const DataNode &grand : child)
			{
				const string &word = grand.Token(0);
				bool error = false;
				Format::Replace(word, [&](const string &s) -> string
					{
						const Phrase *subphrase = GameData::Phrases().Get(s);
						if(subphrase->ReferencesPhrase(this))
						{
							child.PrintTrace("Found recursive phrase reference:");
							error = true;
						}
						return "";
					});
				if(!error)
					part.words.push_back(word);
			}
		}
		else if(child.Token(0) == "phrase")
		{
			for(const DataNode &grand : child)
			{
				const Phrase *subphrase = GameData::Phrases().Get(grand.Token(0));
				if(subphrase->ReferencesPhrase(this))
					child.PrintTrace("Found recursive phrase reference:");
				else
					part.phrases.push_back(subphrase);
			}
		}
		else if(child.Token(0) == "replace")
		{
			for(const DataNode &grand : child)
			{
				const string o(grand.Token(0));
				const string n(grand.Size() >= 2 ? grand.Token(1) : "");
				auto f = [o, n](const string &s) -> string { return Replace(s, o, n); };
				part.replaceRules.emplace_back(f);
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
		
		// If no words, phrases, or replaces were given, discard this part of the phrase.
		if(part.words.empty() && part.phrases.empty() && part.replaceRules.empty())
			parts.back().pop_back();
	}
}



const string &Phrase::Name() const
{
	return name;
}



string Phrase::Get() const
{
	string result;
	if(parts.empty())
		return result;
	
	for(const Part &part : parts[Random::Int(parts.size())])
	{
		if(!part.phrases.empty())
			result += part.phrases[Random::Int(part.phrases.size())]->Get();
		else if(!part.words.empty())
		{
			string word = part.words[Random::Int(part.words.size())];
			word = Format::Replace(word, [&](const string &s) -> string
				{
					const Phrase *subphrase = GameData::Phrases().Get(s);
					return subphrase->Get();
				});
			result += word;
		}
		else if(!part.replaceRules.empty())
			for(auto f : part.replaceRules)
				result = f(result);
	}
	
	return result;
}



bool Phrase::ReferencesPhrase(const Phrase *phrase) const
{
	if(phrase == this)
		return true;
	
	for(const vector<Part> &alternative : parts)
		for(const Part &part : alternative)
			for(const Phrase *subphrase : part.phrases)
				if(subphrase->ReferencesPhrase(phrase))
					return true;
	
	return false;
}
