/* LoadingCircle.cpp
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

#include "LoadingCircle.h"

#include "Angle.h"
#include "Color.h"
#include "shader/PointerShader.h"

using namespace std;



LoadingCircle::LoadingCircle(float size, int ticks, double rotationSpeed)
	: size(size), ticks(ticks), angleOffset(360. / ticks), rotationSpeed(rotationSpeed), rotation(0.)
{
}



void LoadingCircle::Step()
{
	rotation += rotationSpeed;
}



void LoadingCircle::Draw(const Point &position, double progress) const
{
	// Draw the loading circle.
	Angle a(rotation);
	PointerShader::Bind();
	for(int i = 0; i < static_cast<int>(progress * ticks); ++i)
	{
		PointerShader::Add(position, a.Unit(), 8.f, 20.f, size, Color(.5f, 0.f));
		a += angleOffset;
	}
	PointerShader::Unbind();
}
