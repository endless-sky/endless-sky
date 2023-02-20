/* ShipEffectsShader.cpp
Copyright (c) 2014-2023 by Michael Zahniser & Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "ShipEffectsShader.h"

#include "Color.h"
#include "GameData.h"
#include "Government.h"
#include "Logger.h"
#include "Messages.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "Shader.h"
#include "Ship.h"
#include "Sprite.h"

#include <algorithm>
#include <bit>
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
	GLint frameI;
	GLint frameCountI;
	GLint positionI;
	GLint transformI;
	GLint blurI;
	GLint clipI;

	GLint recentHitsCountI;
	GLint recentDamageI;
	GLint recentHitsI;
	GLint shieldColorI;
	GLint ratioI;
	GLint sizeI;

	GLint fastI;

	GLuint vao;
	GLuint vbo;
}

Point ShipEffectsShader::center = Point();

// Initialize the shaders.
void ShipEffectsShader::Init()
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
		"out vec2 shrinkby;\n"

		"void main() {\n"
		"  vec2 blurOff = 2.f * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
		"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);\n"
		"  vec2 texCoord = vert + vec2(.5, .5);\n"
		"  shrinkby = scale;\n"
		"  fragTexCoord = vec2(texCoord.x, min(clip, texCoord.y)) + blurOff;\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment sprite shader\n"
		"precision mediump float;\n"
#ifdef ES_GLES
		"precision mediump sampler2DArray;\n"
#endif
		"uniform sampler2DArray tex;\n"
		"uniform float frame;\n"
		"uniform float frameCount;\n"
		"uniform vec2 blur;\n"
		"const int range = 5;\n"

		"uniform vec2 recentHits[64];\n"
		"uniform float recentDamage[64];\n"
		"uniform int recentHitCount;\n"
		"uniform vec4 shieldColor;\n"
		"uniform float ratio;\n"
		"uniform float size;\n"

		"uniform int isFast;\n"

		"in vec2 fragTexCoord;\n"
		"in vec2 shrinkby;\n"

		"out vec4 finalColor;\n"

		"float first = floor(frame);\n"
		"float second = mod(ceil(frame), frameCount);\n"
		"float fade = frame - first + second;\n"

		"vec4 sampleSmooth(sampler2DArray sampler, vec2 uv)\n"
		"{\n"
		"  return mix( texture(tex, vec3(uv, first)),\n"
		"    texture(tex, vec3(uv, second)), fade);\n"
		"}\n"

		"float stripe(float a, float mod)\n"
		"{\n"
		"  return clamp(sin(a*size*4.) * 2. + mod, 0., 1.);"
		"}\n"

		"float sobellish(vec2 uv)\n"
		"{\n"
		"  float obel = 0.;\n"
		"  for (int x = -3; x <= 3; x++)\n"
		"  {\n"
		"    for (int y = -3; y <= 3; y++)\n"
		"	 {\n"
		"      obel += sampleSmooth(tex, uv + vec2(x, y) / (300.)).a;\n"
		"    }\n"
		"  }\n"
		"  obel /= 49.;\n"
		"  return sqrt(2. * obel + 0.2 / (obel / 2. - .6) + 0.3);\n"
		"}\n"

		"float gridPattern(float f, vec2 uv)\n"
		"{\n"
		"  return f + (f - f * (stripe(uv.x, 1.5) * stripe(uv.y * ratio, 1.5)));\n"
		"}\n"

		"float bounds(float inp, float max) {\n"
		"  if(inp < max/10. || inp > max * 0.9){\n"
		"    return 1.;\n"
		"  }\n"
		"  return 0.;\n"
		"}\n"

		"float trianglePattern(float f, vec2 duv)\n"
		"{\n"
		"  vec2 uv = vec2(duv.x, duv.y * ratio);\n"
		"  vec3 nuv = vec3(mod(uv.x + uv.y, 0.2)*5., mod(uv.y - uv.x, 0.2)*5., mod(uv.x, 0.1)*10.);\n"
		"  float maxa = length(vec3(bounds(nuv.x, 1.), bounds(nuv.y, 1.), bounds(nuv.z, 1.))); \n"
		"  return f + (f - f * maxa);\n"
		"}\n"

		"void main()\n"
		"{\n"
		"  vec2 uv = fragTexCoord;\n"
		"  vec4 color;"
		"  if(isFast == 0)\n"
		"  {\n"
		"    float totalimpact = 1.;"
		"    for(int i = 0; i < recentHitCount; i++)\n"
		"    {\n"
		"      vec2 hitPoint = recentHits[i] + vec2(0.5, 0.5);\n"
		"      totalimpact += recentDamage[i];"
		"      color += shieldColor * recentDamage[i] * clamp(2. - distance(hitPoint, uv)*.04*size, 0., 1.5);\n"
		"    }\n"
		"    color /= totalimpact / 1.4;\n"
		"    color = clamp(color, 0., 1.);\n"
		"    color *= sobellish(uv);\n"
		// TODO: Move switch to uniform, add more patterns
		"    int switchint = 0;\n"
		"    switch(switchint)\n"
		"    {\n"
		"      case 0:\n"
		"        color *= gridPattern(color.a, uv);\n"
		"        break;\n"
		"      case 1:\n"
		"        color *= trianglePattern(color.a, uv);\n"
		"        break;\n"
		"    }\n"
		"  }\n"
		"  else if(recentHitCount != 0)\n"
		"  {\n"
		"    color = sobellish(uv) * shieldColor * recentDamage[0] * 0.4;\n"
		"  }\n"

		"  finalColor = color;\n"
		"}\n"
		"\n";

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");

	recentHitsI = shader.Uniform("recentHits");
	recentDamageI = shader.Uniform("recentDamage");
	recentHitsCountI = shader.Uniform("recentHitCount");
	ratioI = shader.Uniform("ratio");
	sizeI = shader.Uniform("size");

	fastI = shader.Uniform("isFast");

	shieldColorI = shader.Uniform("shieldColor");

	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.55f, -.55f,
		-.55f,  .55f,
		 .55f, -.55f,
		 .55f,  .55f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void ShipEffectsShader::Draw(const Body* body, const Point& position, const vector<pair<Point, double>>* recentHits,
	const float zoom, const float frame, const vector<pair<string, double>> &shieldColor)
{
	if(!body->GetSprite())
		return;

	Bind();
	Add(Prepare(body, position, recentHits, zoom, frame, shieldColor));
	Unbind();
}



void ShipEffectsShader::SetCenter(Point newCenter)
{
	center = newCenter;
}



ShipEffectsShader::EffectItem ShipEffectsShader::Prepare(const Body* body, const Point& position,
	const vector<pair<Point, double>>* recentHits, const float zoom, const float frame, const vector<pair<string, double>> &shieldColor)
{
	if(!body->GetSprite())
		return {};

	EffectItem item;
	item.texture = body->GetSprite()->Texture();
	item.frame = frame;
	item.frameCount = body->GetSprite()->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X() * zoom);
	item.position[1] = static_cast<float>(position.Y() * zoom);

	// Get unit vectors in the direction of the object's width and height.
	double width = body->Width();
	double height = body->Height();
	Point unit = body->Facing().Unit();
	Point uw = unit * width;
	Point uh = unit * height;

	item.size = body->Radius() * 2.;

	// (0, -1) means a zero-degree rotation (since negative Y is up).
	uw *= zoom;
	uh *= zoom;
	item.transform[0] = -uw.Y();
	item.transform[1] = uw.X();
	item.transform[2] = -uh.X();
	item.transform[3] = -uh.Y();

	item.ratio = max(width, height);

	auto recth = recentHits;

	item.recentHits = min(32, static_cast<int>(recth->size()));

	Angle sub = Angle(180.) - body->Facing();
	for(int i = 0; i < static_cast<int>(item.recentHits);)
	{
		const auto newP = sub.Rotate(recth->at(i).first * Point(-1, -1));
		item.recentHitPoints[2 * i] = (newP.X() / ((2 / 1.5) * body->Radius()));
		item.recentHitPoints[2 * i + 1] = (newP.Y() / ((2 / 1.5) * body->Radius()));
		item.recentHitDamage[i] = (min(1., recth->at(i).second));
		//Messages::Add("Hit at " + to_string(newP.X() / body->Radius()) + ", " + to_string(newP.Y() / body->Radius())
		//	+ ", intensity of " + to_string(item.recentHitDamage[i]) + " with count of " + to_string(item.recentHits));
		i++;
	}

	float total = 2.;
	float finalColour[4] = {0, 0, 0, 0};
	for (const auto& it : shieldColor)
	{
		total += it.second;
	}
	const auto colourx = body->GetGovernment()->GetColor().Get();
	const float modix = 2. / total;
	finalColour[0] += colourx[0] * modix;
	finalColour[1] += colourx[1] * modix;
	finalColour[2] += colourx[2] * modix;
	finalColour[3] += colourx[3] * modix * .75;
	for (const auto& it : shieldColor)
	{
		const auto colour = GameData::Colors().Get(it.first)->Get();
		const float modi = it.second / total;
		finalColour[0] += colour[0] * modi;
		finalColour[1] += colour[1] * modi;
		finalColour[2] += colour[2] * modi;
		finalColour[3] += colour[3] * modi;
	}

	copy(finalColour, finalColour + 4, item.shieldColor);


	return item;
}



void ShipEffectsShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = { 2.f / Screen::Width(), -2.f / Screen::Height() };
	glUniform2fv(scaleI, 1, scale);
}



void ShipEffectsShader::Add(const EffectItem& item, bool withBlur)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, item.texture);

	glUniform1f(frameI, item.frame);
	glUniform1f(frameCountI, item.frameCount);
	glUniform2fv(positionI, 1, item.position);
	glUniformMatrix2fv(transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = { 0.f, 0.f };
	glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
	glUniform1f(clipI, item.clip);

	glUniform2fv(recentHitsI, 128, item.recentHitPoints);
	glUniform1fv(recentDamageI, 64, item.recentHitDamage);
	glUniform4fv(shieldColorI, 1, item.shieldColor);
	glUniform1i(recentHitsCountI, item.recentHits);
	glUniform1f(ratioI, item.ratio);
	glUniform1f(sizeI, item.size);
	glUniform1i(fastI, 2 == static_cast<int>(Preferences::GetHitEffects()));
	Logger::LogError(to_string(item.recentHits));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void ShipEffectsShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
