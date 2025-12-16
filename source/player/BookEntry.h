/* BookEntry.h
Copyright (c) 2025 by xobes

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

class Color;
class DataNode;
class DataWriter;
class Point;
class Sprite;
class WrappedText;



// Implement a collection of text and image nodes which form a singular Logbook entry.
class BookEntry {
public:
	typedef std::variant<std::monostate, const Sprite *, std::string> Item;


public:
	bool IsEmpty() const;
	void Load(const DataNode &node, int startAt = 0);
	void Add(const BookEntry &other);

	// When a GameAction is instantiated, substitutions are performed.
	BookEntry Instantiate(const std::map<std::string, std::string> &subs) const;

	void Save(DataWriter &out) const;

	// Returns height.
	int Draw(const Point &topLeft, WrappedText &wrap, const Color &color) const;


private:
	void LoadSingle(const DataNode &node, int startAt = 0);


private:
	std::vector<Item> items;
};
