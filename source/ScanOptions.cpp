/* ScanOptions.cpp
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

#include "ScanOptions.h"

#include <algorithm>

using namespace std;



void ScanOptions::AddOption(ScanType type, const std::shared_ptr<Ship> &ship, double distance, double speed)
{
	options[type].emplace_back(ship, distance, speed);
}



bool ScanOptions::HasOption(ScanType type) const
{
	return options.contains(type);
}



shared_ptr<Ship> ScanOptions::GetClosest(ScanType type) const
{
	auto it = options.find(type);
	if(it == options.end() || it->second.empty())
		return nullptr;
	auto closest = ranges::min_element(it->second,
		[](const Option &a, const Option &b) {
			if(a.distance == b.distance)
				return a.speed > b.speed;
			return a.distance < b.distance;
		}
	);
	return closest->ship;
}



shared_ptr<Ship> ScanOptions::GetStrongest(ScanType type) const
{
	auto it = options.find(type);
	if(it == options.end() || it->second.empty())
		return nullptr;
	auto strongest = ranges::max_element(it->second,
		[](const Option &a, const Option &b) {
			if(a.speed == b.speed)
				return a.distance < b.distance;
			return a.speed > b.speed;
		}
	);
	return strongest->ship;
}



const vector<ScanOptions::Option> &ScanOptions::GetOptions(ScanType type) const
{
	static const vector<Option> EMPTY;
	auto it = options.find(type);
	return it != options.end() ? it->second : EMPTY;
}
