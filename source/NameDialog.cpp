/* NameDialog.cpp
Copyright (c) 2024 by Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "NameDialog.h"

#include "Color.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

void NameDialog::Draw()
{
	Dialog::Draw();

	randomPos = cancelPos - Point(100., 0.);
	SpriteShader::Draw(SpriteSet::Get("ui/dialog cancel"), randomPos);

	const Font &font = FontSet::Get(14);
	static const string RANDOM = "Suggest";
	Point labelPos = randomPos - .5 * Point(font.Width(RANDOM), font.Height());
	font.Draw(RANDOM, labelPos, *GameData::Colors().Get("medium"));
}

bool NameDialog::Click(int x, int y, int clicks)
{
	Point off = Point(x, y) - randomPos;
	if(fabs(off.X()) < 40. && fabs(off.Y()) < 20.)
	{
		// TODO: always chooses human names even for alien ships
		input = GameData::Phrases().Get("civilian")->Get();
		return true;
	}
	return Dialog::Click(x, y, clicks);
}
