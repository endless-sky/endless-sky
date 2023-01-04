/* Phrase.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Phrase.h"

#include "DataNode.h"
#include "text/Format.h"
#include "GameData.h"

using namespace std;

namespace {
	void CombineVectors(vector<string> &base, const vector<string> &toAdd)
	{
		vector<string> result;
		result.reserve(base.size() * toAdd.size());
		for(const string &baseString : base)
			for(const string &toAppend : toAdd)
				result.push_back(baseString + toAppend);
		std::swap(base, result);
	}

	void AppendToVector(vector<string> &base, const string &toAppend)
	{
		for(string &it : base)
			it.append(toAppend);
	}

	void PushToVector(vector<string> &base, const vector<string> &toAdd)
	{
		base.reserve(base.size() + toAdd.size());
		for(const string &toPush : toAdd)
			base.push_back(toPush);
	}
}



Phrase::Phrase(const DataNode &node)
{
	Load(node);
}



void Phrase::Load(const DataNode &node)
{
	// Set the name of this phrase, so we know it has been loaded.
	name = node.Size() >= 2 ? node.Token(1) : "Unnamed Phrase";
	// To avoid a possible parsing ambiguity, the interpolation delimiters
	// may not be used in a Phrase's name.
	if(name.find("${") != string::npos || name.find('}') != string::npos)
	{
		node.PrintTrace("Error: Phrase names may not contain '${' or '}':");
		return;
	}

	sentences.emplace_back(node, this);
	if(sentences.back().empty())
	{
		sentences.pop_back();
		node.PrintTrace("Error: Unable to parse node:");
	}
}



bool Phrase::IsEmpty() const
{
	return sentences.empty();
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
		if(!part.choices.empty())
		{
			const auto &choice = part.choices.Get();
			for(const auto &element : choice)
				result += element.second ? element.second->Get() : element.first;
		}
		else if(!part.replacements.empty())
			for(const auto &pair : part.replacements)
				Format::ReplaceAll(result, pair.first, pair.second);
	}

	return result;
}



vector<string> Phrase::GetAll() const
{
	vector<string> result;

	// Iterate over each sentence in this phrase.
	// Each sentence is its own set of possible results;
	// sentences are not concatenated.
	for(const auto &sentence : sentences)
	{
		vector<string> sentenceVector;
		sentenceVector.emplace_back();

		// Iterate over each part in this sentence.
		// Sequential parts are concatenated to form a sentence.
		for(const auto &part : sentence)
		{
			vector<string> partVector;

			// Iterate over each choice in this part.
			// Choices are added onto the previous parts in parallel, not concatenated with each other.
			for(const auto &choice : part.choices)
			{
				vector<string> choiceVector;
				choiceVector.emplace_back();

				// Iterate over each element in this choice.
				// The elements in a choice are concatenated.
				// Each element is either empty, a string, or a pointer to another phrase.
				for(const auto &element : choice)
				{
					// If this element is a string, append it to each of the partial results for this choice.
					if(!element.first.empty())
						AppendToVector(choiceVector, element.first);
					// If this element references another phrase, get all the possible results of that phrase,
					// and append each of those to each of the existing partial results for this choice.
					else if(element.second)
					{
						vector<string> subPhraseVector = element.second->GetAll();
						CombineVectors(choiceVector, subPhraseVector);
					}
				}

				// Add the new possible options for this part
				// from this choice to the back of the part vector.
				PushToVector(partVector, choiceVector);
			}

			// Append each possible result of this part to each patial sentence.
			CombineVectors(sentenceVector, partVector);
		}

		// Append each possible result for this sentence to the final result vector.
		PushToVector(result, sentenceVector);
	}

	return result;
}



// Inspect this phrase and all its subphrases to determine if a cyclic
// reference exists between this phrase and the other.
bool Phrase::ReferencesPhrase(const Phrase *other) const
{
	if(other == this)
		return true;

	for(const auto &sentence : sentences)
		for(const auto &part : sentence)
			for(const auto &choice : part.choices)
				for(const auto &element : choice)
					if(element.second && element.second->ReferencesPhrase(other))
						return true;

	return false;
}



Phrase::Choice::Choice(const DataNode &node, bool isPhraseName)
{
	// The given datanode should not have any children.
	if(node.HasChildren())
		node.begin()->PrintTrace("Skipping unrecognized child node:");

	if(isPhraseName)
	{
		emplace_back(string{}, GameData::Phrases().Get(node.Token(0)));
		return;
	}

	// This node is a text string that may contain an interpolation request.
	const string &entry = node.Token(0);
	if(entry.empty())
	{
		// A blank choice was desired.
		emplace_back();
		return;
	}

	size_t start = 0;
	while(start < entry.length())
	{
		// Determine if there is an interpolation request in this string.
		size_t left = entry.find("${", start);
		if(left == string::npos)
			break;
		size_t right = entry.find('}', left);
		if(right == string::npos)
			break;

		// Add the text up to the ${, and then add the contained phrase name.
		++right;
		size_t length = right - left;
		auto text = string{entry, start, left - start};
		auto phraseName = string{entry, left + 2, length - 3};
		emplace_back(text, nullptr);
		emplace_back(string{}, GameData::Phrases().Get(phraseName));
		start = right;
	}
	// Add the remaining text to the sequence.
	if(entry.length() - start > 0)
		emplace_back(string{entry, start, entry.length() - start}, nullptr);
}



// Forwarding constructor, for use with emplace/emplace_back.
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
			for(const DataNode &grand : child)
				part.choices.emplace_back((grand.Size() >= 2) ? max<int>(1, grand.Value(1)) : 1, grand);
		else if(child.Token(0) == "phrase")
			for(const DataNode &grand : child)
				part.choices.emplace_back((grand.Size() >= 2) ? max<int>(1, grand.Value(1)) : 1, grand, true);
		else if(child.Token(0) == "replace")
			for(const DataNode &grand : child)
				part.replacements.emplace_back(grand.Token(0), (grand.Size() >= 2) ? grand.Token(1) : string{});
		else
			child.PrintTrace("Skipping unrecognized attribute:");

		// Require any newly added phrases have no recursive references. Any recursions
		// will instead yield an empty string, rather than possibly infinite text.
		for(auto &choice : part.choices)
			for(auto &element : choice)
				if(element.second && element.second->ReferencesPhrase(parent))
				{
					child.PrintTrace("Warning: Replaced recursive '" + element.second->Name() + "' phrase reference with \"\":");
					element.second = nullptr;
				}

		// If no words, phrases, or replaces were given, discard this part of the phrase.
		if(part.choices.empty() && part.replacements.empty())
			pop_back();
	}
}
