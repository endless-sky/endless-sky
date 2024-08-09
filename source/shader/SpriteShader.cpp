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

#include "../Screen.h"
#include "Shader.h"
#include "../Sprite.h"

#include <sstream>
#include <vector>

#ifdef ES_GLES
// ES_GLES always uses the shader, not this, so use a dummy value to compile.
// (the correct value is usually 0x8E46, so don't use that)
#define GL_TEXTURE_SWIZZLE_RGBA 0xBEEF
#endif

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint texI;
	GLint swizzleMaskI;
	GLint useSwizzleMaskI;
	GLint frameI;
	GLint frameCountI;
	GLint positionI;
	GLint transformI;
	GLint blurI;
	GLint clipI;
	GLint alphaI;
	GLint swizzlerI;

	GLuint vao;
	GLuint vbo;

	const int SWIZZLES = 29;
}

// Initialize the shaders.
void SpriteShader::Init()
{

	static const char *vertexCode =
		"// vertex sprite shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform mat2 transform;\n"
		"uniform vec2 blur;\n"
		"uniform float clip;\n"

		"in vec2 vert;\n"
		"out vec2 fragTexCoord;\n"

		"void main() {\n"
		"  vec2 blurOff = 2.f * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
		"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);\n"
		"  vec2 texCoord = vert + vec2(.5, .5);\n"
		"  fragTexCoord = vec2(texCoord.x, min(clip, texCoord.y)) + blurOff;\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment sprite shader\n"
		"precision mediump float;\n"
#ifdef ES_GLES
		"precision mediump sampler2DArray;\n"
#endif
		"uniform sampler2DArray tex;\n"
		"uniform sampler2DArray swizzleMask;\n"
		"uniform int useSwizzleMask;\n"
		"uniform float frame;\n"
		"uniform float frameCount;\n"
		"uniform vec2 blur;\n"
		"uniform int swizzler;\n"
		"uniform float alpha;\n"
		"const int range = 5;\n"

		"in vec2 fragTexCoord;\n"

		"out vec4 finalColor;\n"

		"void main() {\n"
		"  float first = floor(frame);\n"
		"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first;\n"
		"  vec4 color;\n"
		"  if(blur.x == 0.f && blur.y == 0.f)\n"
		"  {\n"
		"    if(fade != 0.f)\n"
		"      color = mix(\n"
		"        texture(tex, vec3(fragTexCoord, first)),\n"
		"        texture(tex, vec3(fragTexCoord, second)), fade);\n"
		"    else\n"
		"      color = texture(tex, vec3(fragTexCoord, first));\n"
		"  }\n"
		"  else\n"
		"  {\n"
		"    color = vec4(0., 0., 0., 0.);\n"
		"    const float divisor = float(range * (range + 2) + 1);\n"
		"    for(int i = -range; i <= range; ++i)\n"
		"    {\n"
		"      float scale = float(range + 1 - abs(i)) / divisor;\n"
		"      vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
		"      if(fade != 0.f)\n"
		"        color += scale * mix(\n"
		"          texture(tex, vec3(coord, first)),\n"
		"          texture(tex, vec3(coord, second)), fade);\n"
		"      else\n"
		"        color += scale * texture(tex, vec3(coord, first));\n"
		"    }\n"
		"  }\n"
		"  vec4 swizzleColor;\n"
		"  switch (swizzler) {\n"
		// 0 red + yellow markings (republic)
		"    case 0:\n"
		"      swizzleColor = color.rgba;\n"
		"      break;\n"
		// 1 red + magenta markings
		"    case 1:\n"
		"      swizzleColor = color.rbga;\n"
		"      break;\n"
		// 2 green + yellow (free worlds)
		"    case 2:\n"
		"      swizzleColor = color.grba;\n"
		"      break;\n"
		// 3 green + cyan
		"    case 3:\n"
		"      swizzleColor = color.brga;\n"
		"      break;\n"
		// 4 blue + magenta (syndicate)
		"    case 4:\n"
		"      swizzleColor = color.gbra;\n"
		"      break;\n"
		// 5 blue + cyan (merchant)
		"    case 5:\n"
		"      swizzleColor = color.bgra;\n"
		"      break;\n"
		// 6 red and black (pirate)
		"    case 6:\n"
		"      swizzleColor = color.gbba;\n"
		"      break;\n"
		// 7 pure red
		"    case 7:\n"
		"      swizzleColor = color.rbba;\n"
		"      break;\n"
		// 8 faded red
		"    case 8:\n"
		"      swizzleColor = color.rgga;\n"
		"      break;\n"
		// 9 pure black
		"    case 9:\n"
		"      swizzleColor = color.bbba;\n"
		"      break;\n"
		// 10 faded black
		"    case 10:\n"
		"      swizzleColor = color.ggga;\n"
		"      break;\n"
		// 11 pure white
		"    case 11:\n"
		"      swizzleColor = color.rrra;\n"
		"      break;\n"
		// 12 darkened blue
		"    case 12:\n"
		"      swizzleColor = color.bbga;\n"
		"      break;\n"
		// 13 pure blue
		"    case 13:\n"
		"      swizzleColor = color.bbra;\n"
		"      break;\n"
		// 14 faded blue
		"    case 14:\n"
		"      swizzleColor = color.ggra;\n"
		"      break;\n"
		// 15 darkened cyan
		"    case 15:\n"
		"      swizzleColor = color.bgga;\n"
		"      break;\n"
		// 16 pure cyan
		"    case 16:\n"
		"      swizzleColor = color.brra;\n"
		"      break;\n"
		// 17 faded cyan
		"    case 17:\n"
		"      swizzleColor = color.grra;\n"
		"      break;\n"
		// 18 darkened green
		"    case 18:\n"
		"      swizzleColor = color.bgba;\n"
		"      break;\n"
		// 19 pure green
		"    case 19:\n"
		"      swizzleColor = color.brba;\n"
		"      break;\n"
		// 20 faded green
		"    case 20:\n"
		"      swizzleColor = color.grga;\n"
		"      break;\n"
		// 21 darkened yellow
		"    case 21:\n"
		"      swizzleColor = color.ggba;\n"
		"      break;\n"
		// 22 pure yellow
		"    case 22:\n"
		"      swizzleColor = color.rrba;\n"
		"      break;\n"
		// 23 faded yellow
		"    case 23:\n"
		"      swizzleColor = color.rrga;\n"
		"      break;\n"
		// 24 darkened magenta
		"    case 24:\n"
		"      swizzleColor = color.gbga;\n"
		"      break;\n"
		// 25 pure magenta
		"    case 25:\n"
		"      swizzleColor = color.rbra;\n"
		"      break;\n"
		// 26 faded magenta
		"    case 26:\n"
		"      swizzleColor = color.rgra;\n"
		"      break;\n"
		// 27 red only (cloaked)
		"    case 27:\n"
		"      swizzleColor = vec4(color.b, 0.f, 0.f, color.a);\n"
		"      break;\n"
		// 28 black only (outline)
		"    case 28:\n"
		"      swizzleColor = vec4(0.f, 0.f, 0.f, color.a);\n"
		"      break;\n"
		"  }\n"
		"  if(useSwizzleMask > 0)\n"
		"  {\n"
		"    float factor = texture(swizzleMask, vec3(fragTexCoord, first)).r;\n"
		"    color = color * factor + swizzleColor * (1.0 - factor);\n"
		"  }\n"
		"  else\n"
		"    color = swizzleColor;\n"
		"  finalColor = color * alpha;\n"
		"}\n";

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	texI = shader.Uniform("tex");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	alphaI = shader.Uniform("alpha");
	swizzlerI = shader.Uniform("swizzler");
	swizzleMaskI = shader.Uniform("swizzleMask");
	useSwizzleMaskI = shader.Uniform("useSwizzleMask");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void SpriteShader::Draw(const Sprite *sprite, const Point &position,
	float zoom, int swizzle, float frame, const Point &unit)
{
	if(!sprite)
		return;

	Bind();
	Add(Prepare(sprite, position, zoom, swizzle, frame, unit));
	Unbind();
}



SpriteShader::Item SpriteShader::Prepare(const Sprite *sprite, const Point &position,
	float zoom, int swizzle, float frame, const Point &unit)
{
	if(!sprite)
		return {};

	Item item;
	item.texture = sprite->Texture();
	item.swizzleMask = sprite->SwizzleMask();
	item.frame = frame;
	item.frameCount = sprite->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());
	// Rotation and scale.
	Point scaledUnit = unit * zoom;
	Point uw = scaledUnit * sprite->Width();
	Point uh = scaledUnit * sprite->Height();
	item.transform[0] = static_cast<float>(-uw.Y());
	item.transform[1] = static_cast<float>(uw.X());
	item.transform[2] = static_cast<float>(-uh.X());
	item.transform[3] = static_cast<float>(-uh.Y());
	// Swizzle.
	item.swizzle = swizzle;

	return item;
}



void SpriteShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void SpriteShader::Add(const Item &item, bool withBlur)
{
	glUniform1i(texI, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, item.texture);

	glUniform1i(swizzleMaskI, 1);
	// Don't mask full color swizzles that always apply to the whole ship sprite.
	glUniform1i(useSwizzleMaskI, item.swizzle >= 27 ? 0 : item.swizzleMask);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, item.swizzleMask);
	glActiveTexture(GL_TEXTURE0);

	glUniform1f(frameI, item.frame);
	glUniform1f(frameCountI, item.frameCount);
	glUniform2fv(positionI, 1, item.position);
	glUniformMatrix2fv(transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
	glUniform1f(clipI, item.clip);
	glUniform1f(alphaI, item.alpha);

	// Bounds check for the swizzle value:
	int swizzle = (static_cast<size_t>(item.swizzle) >= SWIZZLES ? 0 : item.swizzle);
	// Set the color swizzle.
	glUniform1i(swizzlerI, swizzle);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	// Reset the swizzle.
	glUniform1i(swizzlerI, 0);

	glBindVertexArray(0);
	glUseProgram(0);
}
