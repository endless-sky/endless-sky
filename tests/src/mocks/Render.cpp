/* Render.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/Render.h"

#include "../../../source/Body.h"
#include "../../../source/StarField.h"

using namespace std;

namespace {
	StarField background;
}



void Render::Load()
{
}



void Render::LoadShaders(bool useShaderSwizzle)
{
}



double Render::Progress()
{
	return 1.;
}



bool Render::IsLoaded()
{
	return true;
}



void Render::Preload(const Sprite *sprite)
{
}



void Render::FinishLoading()
{
}



const StarField &Render::Background()
{
	return background;
}



void Render::SetHaze(const Sprite *sprite, bool allowAnimation)
{
}



void Render::LoadPlugin(const string &path, const string &name)
{
}
