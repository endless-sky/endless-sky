/* SpriteShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "../../../source/SpriteShader.h"

using namespace std;

namespace {
}

bool SpriteShader::useShaderSwizzle = false;

// Initialize the shaders.
void SpriteShader::Init(bool useShaderSwizzle)
{
	SpriteShader::useShaderSwizzle = useShaderSwizzle;
}



void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle, float frame)
{
}



void SpriteShader::Bind()
{
}



void SpriteShader::Add(const Item &item, bool withBlur)
{
}



void SpriteShader::Unbind()
{
}
