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
	const Government *gov = projectile.GetGovernment();
	const bool isInDanger = projectile.GetWeapon().HullDamage() > flagship->Attributes().Get("hull")/10;

	color = gov->GetColor();

	if(isInDanger && gov->IsEnemy())
		bigText = "!DANGER!";

	if(isTargetingMe)
		smallText = "LOCK";

	if(projectile.TargetGovernment()->IsPlayer())
		color2 = isInDanger ? Color(1.f, 0.1f, .1f, 1.f) : Color(color.Get()[0] * .5f + .5f, color.Get()[1] * .5f + .1f, color.Get()[2] * .5f + .1f);
	else
		color2 = color;

	radius = zoom*projectile.Radius();
}



void AlertLabel::Draw() const
{
	// Draw any active labels.
	const Font &font = FontSet::Get(14);
	const Font &bigFont = FontSet::Get(18);

	double innerAngle = 60.;
	double outerAngle = innerAngle - 360. * GAP / (2. * PI * radius);
	Point unit = Angle(innerAngle).Unit();
	RingShader::Draw(position, radius, 2.f, .9f, color2, 0.f, innerAngle);
	RingShader::Draw(position, radius + GAP, 1.5f, .6f, color, 0.f, outerAngle);

	if(!bigText.empty() || !smallText.empty())
	{
		Point from = position + (radius + INNER_SPACE + LINE_GAP) * unit;
		Point to = from + LINE_LENGTH * unit;
		LineShader::Draw(from, to, 1.3f, color);

		double bigX = to.X() + 2.;
		bigFont.DrawAliased(bigText, bigX, to.Y() - .5 * bigFont.Height(), color2);

		double smallX = to.X() + 4.;
		font.DrawAliased(smallText, smallX, to.Y() + .5 * bigFont.Height() + 1., color);
	}
}
