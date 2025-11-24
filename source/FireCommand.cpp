/* FireCommand.cpp
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FireCommand.h"

#include <cassert>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	template<typename T>
	void SubsetAssign(vector<T> &lhs, const vector<T> &rhs) noexcept
	{
		const auto size = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
		for(size_t i = 0; i < size; ++i)
			lhs[i] = rhs[i];
	}
}



// Sets the specified amount of hardpoints desired.
void FireCommand::SetHardpoints(size_t count)
{
	Clear();
	weapon.Resize(count);
	aim.resize(count);

	assert(aim.size() == count && "aim size must match the requested count");
	assert(weapon.Size() >= aim.size() && "weapon bits must be at least as big as the aim bits");
}



// Assigns the subset of other to this class that is no larger than
// this command's hardpoint size.
void FireCommand::UpdateWith(const FireCommand &other) noexcept
{
	weapon.UpdateWith(other.weapon);
	SubsetAssign(aim, other.aim);
}



// Reset this to an empty command.
void FireCommand::Clear()
{
	weapon.Reset();
	for(auto &it : aim)
		it = '\0';
}



// Check if this command includes a command to fire the given weapon.
bool FireCommand::HasFire(int index) const noexcept
{
	if(!IsIndexValid(index))
		return false;
	return weapon.Test(index);
}



// Add to this set of commands a command to fire the given weapon.
void FireCommand::SetFire(int index) noexcept
{
	if(!IsIndexValid(index))
		return;
	weapon.Set(index);
}



// Check if any weapons are firing.
bool FireCommand::IsFiring() const noexcept
{
	return weapon.Any();
}



// Gets the current turn rate of the turret at the given weapon index.
double FireCommand::Aim(int index) const noexcept
{
	if(!IsIndexValid(index))
		return 0;
	return aim[index] / 127.;
}


// Set the turn rate of the turret with the given weapon index. A value of
// -1 or 1 means to turn at the full speed the turret is capable of.
void FireCommand::SetAim(int index, double amount) noexcept
{
	if(!IsIndexValid(index))
		return;
	aim[index] = round(127. * max(-1., min(1., amount)));
}



bool FireCommand::IsIndexValid(int index) const noexcept
{
	return index >= 0 && static_cast<size_t>(index) < aim.size();
}
