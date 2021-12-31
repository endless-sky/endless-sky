/* FireCommand.cpp
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FireCommand.h"

#include <cmath>
#include <vector>

using namespace std;

namespace {
	template <typename T>
	void SubsetAssign(std::vector<T> &lhs, const std::vector<T> &rhs)
	{
		const auto size = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
		for(size_t i = 0; i < size; ++i)
			lhs[i] = rhs[i];
	}
}



// Sets the specified amount of hardpoints desired.
void FireCommand::SetHardpoints(size_t count)
{
	weapon.Resize(count);
	aim.resize(count);
}



// Assigns the subset of other to this class that is no larger than
// this command's hardpoint size.
void FireCommand::AssignSubsetOf(const FireCommand &other)
{
	SubsetAssign(weapon.Bits(), other.weapon.Bits());
	SubsetAssign(aim, other.aim);
}



// Reset this to an empty command.
void FireCommand::Clear()
{
	weapon.Clear();
	aim.clear();
}



// Check if this command includes a command to fire the given weapon.
bool FireCommand::HasFire(int index) const
{
	if(index < 0 || index >= static_cast<int>(weapon.Size()))
		return false;
	return weapon.Test(index);
}



// Add to this set of commands a command to fire the given weapon.
void FireCommand::SetFire(int index)
{
	if(index < 0 || index >= static_cast<int>(weapon.Size()))
		return;
	weapon.Set(index);
}



// Check if any weapons are firing.
bool FireCommand::IsFiring() const
{
	return weapon.Any();
}



// Set the turn rate of the turret with the given weapon index. A value of
// -1 or 1 means to turn at the full speed the turret is capable of.
double FireCommand::Aim(int index) const
{
	if(index < 0 || index >= static_cast<int>(aim.size()))
		return 0;
	return aim[index] / 127.;
}



void FireCommand::SetAim(int index, double amount)
{
	if(index < 0 || index >= static_cast<int>(aim.size()))
		return;
	aim[index] = round(127. * max(-1., min(1., amount)));
}
