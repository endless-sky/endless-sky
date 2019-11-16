/* Phrase.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PHRASE_H_
#define PHRASE_H_

#include <functional>
#include <string>
#include <utility>
#include <vector>

class DataNode;



// Class representing a set of rules for generating text strings from words.
class Phrase {
public:
	void Load(const DataNode &node);
	
	const std::string &Name() const;
	std::string Get() const;
	
	
private:
	friend class Sentense;
	bool ReferencesPhrase(const Phrase *phrase) const;
	
	
public:
	// Option represents a child node in a "word" or "phrase" node.
	using Option = std::vector<std::pair<std::string, const Phrase*>>;
	
	
private:
	// Part represents a "word", "phrase", or "replace" in a phrase node.
	class Part {
	public:
		std::vector<Option> options;
		std::vector<std::function<std::string(const std::string&)>> replaceRules;
	};
	
	// Sentense represents a phrase node.
	class Sentense {
	public:
		void Load(const DataNode &node, const Phrase* parent);
		
		std::vector<Part> parts;
	};
	
	
private:
	std::string name;
	// Each time this phrase is defined, a new sentence is created.
	std::vector<Sentense> sentenses;
};



#endif
