/* CategoryList.h
Copyright (c) 2022 by Amazinite

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

#include <iterator>
#include <string>
#include <utility>
#include <vector>

class DataNode;



// A CategoryList is a list of names that are associated to a Category of items (e.g. ships
// or outfits). Categories within the list are sorted by the precedence of each Category.
// Any conflicting precedences are resolved by sorting the names of the Categories
// alphabetically.
class CategoryList {
public:
	// A Category is a string with some precedence to it. The precedence is used to sort
	// the Category within the CategoryList. Only the CategoryList has access to the
	// precedence of each Category. All outside classes can only see the Category's
	// name.
	class Category {
	public:
		Category(const std::string &name, int precedence) : name(name), precedence(precedence) {}
		const std::string &Name() const { return name; }
		const bool operator<(const Category &other) const { return SortHelper(*this, other); }
		const bool operator()(const Category &a, const Category &b) const { return SortHelper(a, b); }

	private:
		static const bool SortHelper(const Category &a, const Category &b);


	private:
		friend class CategoryList;
		std::string name;
		int precedence = 0;
	};

public:
	CategoryList() = default;

	void Load(const DataNode &node);

	// Sort the CategoryList. Categories are sorted by precedence. If multiple Categories
	// share the same precedence then they are sorted alphabetically.
	void Sort();

	// Determine if the CategoryList contains a Category with the given name.
	bool Contains(const std::string &name) const;

	const Category GetCategory(const std::string &name) const;

	typename std::vector<Category>::iterator begin() noexcept { return list.begin(); }
	typename std::vector<Category>::const_iterator begin() const noexcept { return list.begin(); }
	typename std::vector<Category>::iterator end() noexcept { return list.end(); }
	typename std::vector<Category>::const_iterator end() const noexcept { return list.end(); }


private:
	std::vector<Category> list;
	int currentPrecedence = 0;
};
