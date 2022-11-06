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
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "Government.h"
#include "LineShader.h"
#include "Projectile.h"
#include "pi.h"
#include "PointerShader.h"
#include "RingShader.h"
#include "Ship.h"
#include "Sprite.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	const double DANGEROUS_ABOVE = .1;
}



AlertLabel::AlertLabel(const Point &position, const Projectile &projectile, const shared_ptr<Ship> &flagship, double zoom)
		: position(position), zoom(zoom)
{
	isTargetingFlagship = projectile.TargetPtr() == flagship;
	double maxHP = flagship->Attributes().Get("hull") + flagship->Attributes().Get("shield");
	double missileDamage = projectile.GetWeapon().HullDamage() + projectile.GetWeapon().ShieldDamage();
	bool isDangerous = (missileDamage / maxHP) > DANGEROUS_ABOVE;

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
	double angle[3] = {330., 210., 90.};
	for(int i = 0; i < 3; i++)
	{
		RingShader::Draw(position * zoom, radius, 1.2f, .16f, *color, 0.f, angle[i] + rotation);
		if(isTargetingFlagship)
			PointerShader::Draw(position * zoom, Angle(angle[i] + 30. + rotation).Unit(), 7.5f, (i ? 10.f : 22.f)*zoom, radius + (i ? 10 : 20)*zoom, *color);
	}
}
