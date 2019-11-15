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

namespace {
	// Parse a word and decompose it into a vector of texts and phrase names.
	void ParseWord(const string &word, vector<pair<string, const Phrase*>> &out)
	{
		size_t start = 0;
		while(start < word.length())
		{
			size_t left = word.find("${", start);
			if(left == string::npos)
				break;
			
			size_t right = word.find('}', left);
			if(right == string::npos)
				break;
			
			++right;
			size_t length = right - left;
			const string leftText(word, start, left - start);
			const string phraseName(word, left + 2, length - 3);
			out.emplace_back(leftText, nullptr);
			out.emplace_back("", GameData::Phrases().Get(phraseName));
			start = right;
		}
		
		if(word.length() - start > 0)
			out.emplace_back(string(word, start, word.length() - start), nullptr);
	}
	
	
	
	// Replace oldStr with newStr in target.
	string Replace(const string &target, const string &oldStr, const string &newStr)
	{
		const size_t oldSize = oldStr.size();
		const size_t newSize = newStr.size();
		string result = target;
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
				part.words.emplace_back();
				ParseWord(grand.Token(0), part.words.back());
			}
		}
		else if(child.Token(0) == "phrase")
			for(const DataNode &grand : child)
				part.words.push_back({make_pair("", GameData::Phrases().Get(grand.Token(0)))});
		else if(child.Token(0) == "replace")
		{
			for(const DataNode &grand : child)
			{
				const string oldStr(grand.Token(0));
				const string newStr(grand.Size() >= 2 ? grand.Token(1) : "");
				auto f = [oldStr, newStr](const string &s) -> string
					{
						return Replace(s, oldStr, newStr);
					};
				part.replaceRules.emplace_back(f);
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
		
		// Confirm the phrases have no recursive phrase reference.
		for(auto &part : parts.back())
			for(auto &word : part.words)
				for(auto &textOrPhrase : word)
					if(textOrPhrase.second)
					{
						const string &phraseName = textOrPhrase.second->Name();
						const Phrase *subphrase = GameData::Phrases().Get(phraseName);
						if(subphrase->ReferencesPhrase(this))
						{
							child.PrintTrace("Found recursive phrase reference:");
							textOrPhrase.second = nullptr;
						}
					}
		
		// If no words, phrases, or replaces were given, discard this part of the phrase.
		if(part.words.empty() && part.replaceRules.empty())
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
		if(!part.words.empty())
		{
			const auto &sequenceOfTextsAndPhrases = part.words[Random::Int(part.words.size())];
			for(const auto &textOrPhrase : sequenceOfTextsAndPhrases)
				if(textOrPhrase.second)
					result += textOrPhrase.second->Get();
				else
					result += textOrPhrase.first;
		}
		else if(!part.replaceRules.empty())
			for(const auto &f : part.replaceRules)
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
			for(const auto &word : part.words)
				for(const auto &subphrase : word)
					if(subphrase.second && subphrase.second->ReferencesPhrase(phrase))
						return true;
	
	return false;
}
