/* ConditionContext.cpp
Copyright (c) 2014-2025 by Michael Zahniser and others

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ConditionContext.h"

#include "Ship.h"

using namespace std;

const Ship * ConditionContext::getHailingShip() const
{
	return nullptr;
}

ConditionContextHailing::ConditionContextHailing(const Ship &hailingShip):
	hailingShip(hailingShip)
{
}

const Ship * ConditionContextHailing::getHailingShip() const
{
	return &hailingShip;
}
