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
	void ParseWord(const string &word, Phrase::Option &out)
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
	// To avoid a possible parsing ambiguity, the interpolation delimiters
	// may not be used in a Phrase's name.
	if(name.find("${") != string::npos || name.find('}') != string::npos)
	{
		node.PrintTrace("Phrase names may not contain '${' or '}':");
		return;
	}
	
	sentences.emplace_back(node, this);
	if(sentences.back().empty())
	{
		sentences.pop_back();
		node.PrintTrace("Skipping unparseable node:");
	}
}



// Get the name associated with the node this phrase was instantiated
// from, or "Unnamed Phrase" if it was anonymously defined.
const string &Phrase::Name() const
{
	return name;
}



// Get a random sentence's text.
string Phrase::Get() const
{
	string result;
	if(sentences.empty())
		return result;
	
	for(const auto &part : sentences[Random::Int(sentences.size())])
	{
		if(!part.options.empty())
		{
			const Option &option = part.options[Random::Int(part.options.size())];
			for(const auto &textOrPhrase : option)
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



// Inspect this phrase and all its subphrases to determine if a cyclic
// reference exists between this phrase and the other.
bool Phrase::ReferencesPhrase(const Phrase *other) const
{
	if(other == this)
		return true;
	
	for(const Sentence &alternative : sentences)
		for(const auto &part : alternative)
			for(const auto &option : part.options)
				for(const auto &subphrase : option)
					if(subphrase.second && subphrase.second->ReferencesPhrase(other))
						return true;
	
	return false;
}



Phrase::Sentence::Sentence(const DataNode &node, const Phrase *parent)
{
	Load(node, parent);
}



// Parse the children of the given node to populate the sentence's structure.
void Phrase::Sentence::Load(const DataNode &node, const Phrase *parent)
{
	for(const DataNode &child : node)
	{
		if(!child.HasChildren())
		{
			child.PrintTrace("Skipping node with no children:");
			continue;
		}
		
		emplace_back();
		auto &part = back();
		
		if(child.Token(0) == "word")
		{
			for(const DataNode &grand : child)
			{
				part.options.emplace_back();
				ParseWord(grand.Token(0), part.options.back());
			}
		}
		else if(child.Token(0) == "phrase")
			for(const DataNode &grand : child)
				part.options.push_back({make_pair("", GameData::Phrases().Get(grand.Token(0)))});
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
		for(auto &option : part.options)
			for(auto &textOrPhrase : option)
				if(textOrPhrase.second)
				{
					const string &phraseName = textOrPhrase.second->Name();
					const Phrase *subphrase = GameData::Phrases().Get(phraseName);
					if(subphrase->ReferencesPhrase(parent))
					{
						child.PrintTrace("Found recursive phrase reference:");
						textOrPhrase.second = nullptr;
					}
				}
		
		// If no words, phrases, or replaces were given, discard this part of the phrase.
		if(part.options.empty() && part.replaceRules.empty())
			pop_back();
	}
}
