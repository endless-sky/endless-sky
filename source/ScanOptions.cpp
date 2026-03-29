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

using namespace std;



void ScanOptions::AddOption(ScanType type, const std::shared_ptr<Ship> &ship, double distance)
{
	auto it = closest.find(type);
	if(it == closest.end() || it->second.second > distance)
		closest[type] = make_pair(ship, distance);
}



bool ScanOptions::HasOption(ScanType type) const
{
	return closest.contains(type);
}



shared_ptr<Ship> ScanOptions::GetOption(ScanType type) const
{
	auto it = closest.find(type);
	return it != closest.end() ? it->second.first : nullptr;
}
