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
#include "Government.h"
#include "LineShader.h"
#include "Projectile.h"
#include "pi.h"
#include "RingShader.h"
#include "Ship.h"
#include "Sprite.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	const double LINE_ANGLE[4] = {60., 120., 300., 240.};
	const double LINE_LENGTH = 6.;
	const double INNER_SPACE = 1.;
	const double LINE_GAP = 1.;
	const double GAP = 4.;
	const double MIN_DISTANCE = 1.;
}



AlertLabel::AlertLabel(Point &position, const Projectile &projectile, shared_ptr<Ship> flagship, double zoom) : position(position * zoom)
{

	const bool isTargetingMe = projectile.TargetPtr() == flagship;
	const bool isInDanger = projectile.GetWeapon().HullDamage() > flagship->Attributes().Get("hull")/10;
	damagePercent = projectile.GetWeapon().HullDamage() / flagship->Attributes().Get("Hull");

	if(isTargetingMe)
		color = Color(.8f, .8f, .4f, .5f);
	else
		color = Color(1.f, .96f, .37f, .5f);

	if(isInDanger)
		color = Color(1.f, .6f, .4f, .5f);

	color2 = isInDanger ? color : Color(0.f, 0.f);

	radius = zoom*projectile.Radius();
}



void AlertLabel::Draw() const
{
	double innerAngle = 60.;
	double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	RingShader::Draw(position, radius, 2.f, .9f, color, 0.f, innerAngle);
	RingShader::Draw(position, radius + GAP, 15 * damagePercent, .6f, color2, 0.f, outerAngle);
}
