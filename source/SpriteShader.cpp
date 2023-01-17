/* SpriteShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SpriteShader.h"

#ifdef ES_GLES
// ES_GLES always uses the shader, not this, so use a dummy value to compile.
// (the correct value is usually 0x8E46, so don't use that)
#define GL_TEXTURE_SWIZZLE_RGBA 0xBEEF
#endif

const vector<vector<GLint>> SpriteShader::ShaderState::SWIZZLE = {
	{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA}, // 0 red + yellow markings (republic)
	{GL_RED, GL_BLUE, GL_GREEN, GL_ALPHA}, // 1 red + magenta markings
	{GL_GREEN, GL_RED, GL_BLUE, GL_ALPHA}, // 2 green + yellow (free worlds)
	{GL_BLUE, GL_RED, GL_GREEN, GL_ALPHA}, // 3 green + cyan
	{GL_GREEN, GL_BLUE, GL_RED, GL_ALPHA}, // 4 blue + magenta (syndicate)
	{GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA}, // 5 blue + cyan (merchant)
	{GL_GREEN, GL_BLUE, GL_BLUE, GL_ALPHA}, // 6 red and black (pirate)
	{GL_RED, GL_BLUE, GL_BLUE, GL_ALPHA}, // 7 pure red
	{GL_RED, GL_GREEN, GL_GREEN, GL_ALPHA}, // 8 faded red
	{GL_BLUE, GL_BLUE, GL_BLUE, GL_ALPHA}, // 9 pure black
	{GL_GREEN, GL_GREEN, GL_GREEN, GL_ALPHA}, // 10 faded black
	{GL_RED, GL_RED, GL_RED, GL_ALPHA}, // 11 pure white
	{GL_BLUE, GL_BLUE, GL_GREEN, GL_ALPHA}, // 12 darkened blue
	{GL_BLUE, GL_BLUE, GL_RED, GL_ALPHA}, // 13 pure blue
	{GL_GREEN, GL_GREEN, GL_RED, GL_ALPHA}, // 14 faded blue
	{GL_BLUE, GL_GREEN, GL_GREEN, GL_ALPHA}, // 15 darkened cyan
	{GL_BLUE, GL_RED, GL_RED, GL_ALPHA}, // 16 pure cyan
	{GL_GREEN, GL_RED, GL_RED, GL_ALPHA}, // 17 faded cyan
	{GL_BLUE, GL_GREEN, GL_BLUE, GL_ALPHA}, // 18 darkened green
	{GL_BLUE, GL_RED, GL_BLUE, GL_ALPHA}, // 19 pure green
	{GL_GREEN, GL_RED, GL_GREEN, GL_ALPHA}, // 20 faded green
	{GL_GREEN, GL_GREEN, GL_BLUE, GL_ALPHA}, // 21 darkened yellow
	{GL_RED, GL_RED, GL_BLUE, GL_ALPHA}, // 22 pure yellow
	{GL_RED, GL_RED, GL_GREEN, GL_ALPHA}, // 23 faded yellow
	{GL_GREEN, GL_BLUE, GL_GREEN, GL_ALPHA}, // 24 darkened magenta
	{GL_RED, GL_BLUE, GL_RED, GL_ALPHA}, // 25 pure magenta
	{GL_RED, GL_GREEN, GL_RED, GL_ALPHA}, // 26 faded magenta
	{GL_BLUE, GL_ZERO, GL_ZERO, GL_ALPHA}, // 27 red only (cloaked)
	{GL_ZERO, GL_ZERO, GL_ZERO, GL_ALPHA} // 28 black only (outline)
};

// Initialize the shaders.
void SpriteShader::Init(bool useShaderSwizzle)
{
	SpriteShader::UISpace::Init(useShaderSwizzle);
	SpriteShader::ViewSpace::Init(useShaderSwizzle);
}
