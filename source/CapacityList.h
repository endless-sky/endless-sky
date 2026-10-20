/* CapacityList.h
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

#pragma once

#include <iterator>
#include <string>
#include <vector>

class DataNode;



// The CapacityList is a mapping of capacity attributes to how those attributes appear in ShipInfoDisplay.
// Capacities may be always displayed, like outfit, weapon, and engine space, or may only optionally display
// if they are present.
class CapacityList {
public:
	// A Capacity is a mapping of a backing attribute to how that capacity displays in
	// ShipInfoDisplay. Capacities have a precedence that determines the order they're
	// displayed in, and
	class Capacity {
	public:
		Capacity(bool required, const std::string &name, const std::string &attribute, int precedence)
			: required(required), displayName(name), backingAttribute(attribute), precedence(precedence) {}
		bool Required() const { return required; }
		const std::string &Name() const { return displayName; }
		const std::string &Attribute() const { return backingAttribute; }
		bool operator<(const Capacity &other) const { return SortHelper(*this, other); }
		bool operator()(const Capacity &a, const Capacity &b) const { return SortHelper(a, b); }

	private:
		static bool SortHelper(const Capacity &a, const Capacity &b);

	private:
		friend class CapacityList;
		bool required = false;
		std::string displayName;
		std::string backingAttribute;
		int precedence = 0;
	};


public:
	CapacityList() = default;

	void Load(const DataNode &node);
	void Clear();

	// Sort the CapacityList. Capacities are sorted by precedence. If multiple Capacities
	// share the same precedence then they are sorted alphabetically.
	void Sort();

	typename std::vector<Capacity>::iterator begin() noexcept { return list.begin(); }
	typename std::vector<Capacity>::const_iterator begin() const noexcept { return list.begin(); }
	typename std::vector<Capacity>::iterator end() noexcept { return list.end(); }
	typename std::vector<Capacity>::const_iterator end() const noexcept { return list.end(); }


private:
	std::vector<Capacity> list;
	int currentPrecedence = 0;
};
