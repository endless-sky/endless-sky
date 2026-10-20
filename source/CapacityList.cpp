/* CapacityList.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "CapacityList.h"

#include "DataNode.h"

#include <algorithm>
#include <ranges>

using namespace std;



bool CapacityList::Capacity::SortHelper(const CapacityList::Capacity &a, const CapacityList::Capacity &b)
{
	if(a.precedence == b.precedence)
		return a.displayName < b.displayName;
	return a.precedence < b.precedence;
}



void CapacityList::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Size() < 3)
		{
			child.PrintTrace("Expected capacity to have at least three tokens:");
			continue;
		}
		const string &display = child.Token(0);
		if(display != "required" && display != "optional")
		{
			child.PrintTrace("Skipping unrecognized attribute:");
			continue;
		}
		// Use the given precedence. If no precedence is given, use the previous
		// precedence + 1.
		if(child.Size() >= 4)
			currentPrecedence = child.Value(3);
		Capacity cap(display == "required", child.Token(1), child.Token(2), currentPrecedence++);

		// If a given capacity name already exists, its precedence will be updated.
		auto it = ranges::find_if(list, [&cap](const Capacity &c) noexcept -> bool {
			return cap.displayName == c.displayName;
		});
		if(it != list.end())
			it->precedence = cap.precedence;
		else
			list.push_back(cap);
	}
}



void CapacityList::Clear()
{
	list.clear();
	currentPrecedence = 0;
}



void CapacityList::Sort()
{
	sort(list.begin(), list.end());
}
