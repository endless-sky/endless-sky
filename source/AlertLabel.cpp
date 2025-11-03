/* AlertLabel.cpp
Copyright (c) 2022 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AlertLabel.h"

#include "Angle.h"
#include "GameData.h"
#include "shader/PointerShader.h"
#include "Projectile.h"
#include "shader/RingShader.h"
#include "Ship.h"
#include "Weapon.h"

#include <algorithm>

using namespace std;

namespace {
	const double DANGEROUS_ABOVE = .1;
}



AlertLabel::AlertLabel(const Point &position, const Projectile &projectile, const shared_ptr<Ship> &flagship,
		double zoom)
	: position(position), zoom(zoom)
{
	bool isDangerous = false;
	isTargetingFlagship = false;
	if(flagship)
	{
		isTargetingFlagship = projectile.TargetPtr() == flagship;
		double maxHP = flagship->MaxHull() + flagship->MaxShields();
		double missileDamage = projectile.GetWeapon().HullDamage() + projectile.GetWeapon().ShieldDamage();
		isDangerous = (missileDamage / maxHP) > DANGEROUS_ABOVE;
	}

	if(isDangerous)
		color = GameData::Colors().Get("missile dangerous");
	else if(isTargetingFlagship)
		color = GameData::Colors().Get("missile locked");
	else
		color = GameData::Colors().Get("missile enemy");

	radius = zoom * projectile.Radius() * 0.75;
	rotation = projectile.Facing().Degrees();
}



void AlertLabel::Draw() const
{
	const double angle[3] = {330., 210., 90.};
	for(int i = 0; i < 3; i++)
	{
		RingShader::Draw(position * zoom, radius, 1.2f, .16f, *color, 0.f, angle[i] + rotation);
		if(isTargetingFlagship)
			PointerShader::Draw(position * zoom, Angle(angle[i] + 30. + rotation).Unit(),
				7.5f, (i ? 10.f : 22.f) * zoom, radius + (i ? 10.f : 20.f) * zoom, *color);
	}
}
