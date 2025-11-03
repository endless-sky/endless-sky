/* MenuAnimationPanel.cpp
Copyright (c) 2022 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "MenuAnimationPanel.h"

#include "Angle.h"
#include "audio/Audio.h"
#include "Color.h"
#include "shader/PointerShader.h"
#include "UI.h"



MenuAnimationPanel::MenuAnimationPanel()
{
	SetTrapAllEvents(false);

	Audio::Play(Audio::Get("landing"), SoundCategory::UI);
}



void MenuAnimationPanel::Step()
{
	alpha -= .02f;
	// Kill this panel if the animation is done.
	if(alpha <= 0.f)
		GetUI()->Pop(this);
}



void MenuAnimationPanel::Draw()
{
	// Draw the shrinking loading circle.
	Angle da(6.);
	Angle a(0.);
	Color color(.5f * alpha, 0.f);
	PointerShader::Bind();
	for(int i = 0; i < 60; ++i)
	{
		PointerShader::Add(Point(), a.Unit(), 8.f, 20.f, 140.f * alpha, color);
		a += da;
	}
	PointerShader::Unbind();
}
