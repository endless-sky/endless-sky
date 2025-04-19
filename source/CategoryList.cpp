/* CategoryList.cpp
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

#include "CategoryList.h"

#include "DataNode.h"

#include <algorithm>
#include <limits>

using namespace std;



const bool CategoryList::Category::SortHelper(const CategoryList::Category &a, const CategoryList::Category &b)
{
	if(a.precedence == b.precedence)
		return a.name < b.name;
	return a.precedence < b.precedence;
}



void CategoryList::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		// Use the given precedence. If no precedence is given, use the previous
		// precedence + 1.
		if(child.Size() > 1)
			currentPrecedence = child.Value(1);
		Category cat(child.Token(0), currentPrecedence++);

		// If a given category name already exists, its precedence will be updated.
		auto it = find_if(list.begin(), list.end(),
			[&cat](const Category &c) noexcept -> bool { return cat.name == c.name; });
		if(it != list.end())
			it->precedence = cat.precedence;
		else
			list.push_back(cat);
	}
}



// Sort the CategoryList. Categories are sorted by precedence. If multiple categories
// share the same precedence then they are sorted alphabetically.
void CategoryList::Sort()
{
	sort(list.begin(), list.end());
}



// Determine if the CategoryList contains a Category with the given name.
bool CategoryList::Contains(const string &name) const
{
	const auto it = find_if(list.begin(), list.end(),
		[&name](const Category &c) noexcept -> bool { return name == c.name; });
	return it != list.end();
}



const CategoryList::Category CategoryList::GetCategory(const string &name) const
{
	const auto it = find_if(list.begin(), list.end(),
		[&name](const Category &c) noexcept -> bool { return name == c.name; });
	if(it != list.end())
		return *it;
	return Category("", numeric_limits<int>::max());
}
