/* ShadowedSpriteShader.cpp
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

#include "ShadowedSpriteShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

#include "Messages.h"

#include <limits>
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
	GLint alphaI;
	GLint swizzlerI;

	GLint spriteIndexI;
	GLint worldPositionI;

	GLint subLightPosI;
	GLint subLightColI;
//	GLint subLightRadiusI;

	GLuint vao;
	GLuint vbo;

	struct Light{
		Point position = Point();
		Color color = Color(1., 0.);
		double radius = 0.;
	};
	static std::vector<Light> lights;

	const vector<vector<GLint>> SWIZZLE = {
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
}



void ShadowedSpriteShader::AddLight(const Point &position, const Color color, const double radius)
{
	Light newLight;
	newLight.position = position;
	newLight.color = color;
	newLight.radius = radius;
	lights.push_back(newLight);
}



void ShadowedSpriteShader::ClearLights()
{
	lights.clear();
}




bool ShadowedSpriteShader::useShaderSwizzle = false;

// Initialize the shaders.
void ShadowedSpriteShader::Init(bool useShaderSwizzle)
{
	ShadowedSpriteShader::useShaderSwizzle = useShaderSwizzle;

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

	ostringstream fragmentCodeStream;
	fragmentCodeStream <<
		"// fragment sprite shader\n"
		"precision mediump float;\n"
#ifdef ES_GLES
		"precision mediump sampler2DArray;\n"
#endif
		"uniform sampler2DArray tex;\n"
		"uniform sampler2DArray normal;\n"
		"uniform sampler2DArray base;\n"
		"uniform sampler2DArray emit;\n"
		"uniform vec3 worldPosition;\n"
		"uniform vec3 subLightPos;\n"
		"uniform vec4 subLightCol;\n"
//		"uniform float subLightRadius;\n"
		"uniform float frame;\n"
		"uniform float frameCount;\n"
		"uniform int spriteIndex;\n"
		"uniform vec2 blur;\n";
	if(useShaderSwizzle) fragmentCodeStream <<
		"uniform int swizzler;\n";
	fragmentCodeStream <<
		"uniform float alpha;\n"
		"const int range = 5;\n"

		"in vec2 fragTexCoord;\n"

		"out vec4 finalColor;\n"

		"void main() {\n"
		"  float first = floor(frame);\n"
		"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first;\n"
		"  vec4 color;\n"
		"  if(spriteIndex > 2)\n"
		"  {\n"
		"    vec4 normCol = texture(normal, vec3(fragTexCoord, first));\n"
		"    vec4 texCol = texture(tex, vec3(fragTexCoord, first));\n"
		"    vec3 lightVector = normalize(worldPosition);\n"
		"    vec3 subLightVector = normalize(subLightPos);\n"
		"	 if(spriteIndex == 7 || spriteIndex == 15)\n"
		"	 {\n"
		"      if(blur.x == 0.f && blur.y == 0.f)\n"
		"      {\n"
		"        if(fade != 0.f)\n"
		"          color = mix(\n"
		"            texture(base, vec3(fragTexCoord, first)),\n"
		"            texture(base, vec3(fragTexCoord, second)), fade);\n"
		"        else\n"
		"          color = texture(base, vec3(fragTexCoord, first));\n"
		"      }\n"
		"      else\n"
		"      {\n"
		"        color = vec4(0., 0., 0., 0.);\n"
		"        const float divisor = float(range * (range + 2) + 1);\n"
		"        for(int i = -range; i <= range; ++i)\n"
		"        {\n"
		"          float scale = float(range + 1 - abs(i)) / divisor;\n"
		"          vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
		"          if(fade != 0.f)\n"
		"            color += scale * mix(\n"
		"              texture(base, vec3(coord, first)),\n"
		"              texture(base, vec3(coord, second)), fade);\n"
		"          else\n"
		"            color += scale * texture(base, vec3(coord, first));\n"
		"        }\n"
		"      }\n"
		"	 }\n"
		"    else\n"
		"	 {\n"
		"		vec3 ogSunVector = vec3(-0.6, -0.7, 0.355);\n"
		"	    vec3 mNormal = vec3(normCol.x - 0.5f, normCol.y - 0.5f, normCol.z - 0.5f) * 3.f;\n"
		"	    float dotProd = max(dot(mNormal, ogSunVector) + 0.2f, 0.f);\n"
		"	    color = vec4(texCol.rgb + vec3(texture(tex, vec3(1.f-fragTexCoord.x, fragTexCoord.y, first)).rgb*dotProd), texCol.a);\n"
		"	 }\n"
		"    vec3 norm = vec3(normCol.x - 0.5f, normCol.y - 0.5f, normCol.z - 0.5f) * 4.f;\n"
		"    float dotP = 0.5f + (0.5f * dot(norm, lightVector));\n"
		"    float dotP2 = min(max(0.5f + (0.5f * dot(norm, subLightVector)), 0.f) / length(subLightPos), 1.f);\n"
		"    color = color * vec4(vec3(dotP * texCol.a) + (dotP2 * subLightCol.rgb * subLightCol.a), texCol.a);\n"
		"    color = vec4(texture(emit, vec3(fragTexCoord, first)).rgb + color.rgb, texCol.a);\n"
		"  }\n"
		"  else\n"
		"  {\n"
		"    if(blur.x == 0.f && blur.y == 0.f)\n"
		"    {\n"
		"      if(fade != 0.f)\n"
		"        color = mix(\n"
		"          texture(tex, vec3(fragTexCoord, first)),\n"
		"          texture(tex, vec3(fragTexCoord, second)), fade);\n"
		"      else\n"
		"        color = texture(tex, vec3(fragTexCoord, first));\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"      color = vec4(0., 0., 0., 0.);\n"
		"      const float divisor = float(range * (range + 2) + 1);\n"
		"      for(int i = -range; i <= range; ++i)\n"
		"      {\n"
		"        float scale = float(range + 1 - abs(i)) / divisor;\n"
		"        vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
		"        if(fade != 0.f)\n"
		"          color += scale * mix(\n"
		"            texture(tex, vec3(coord, first)),\n"
		"            texture(tex, vec3(coord, second)), fade);\n"
		"        else\n"
		"          color += scale * texture(tex, vec3(coord, first));\n"
		"      }\n"
		"    }\n"
		"  }\n";

	// Only included when hardware swizzle not supported, GL <3.3 and GLES
	if(useShaderSwizzle)
	{
		fragmentCodeStream <<
		"  switch (swizzler) {\n"
		"    case 0:\n"
		"      color = color.rgba;\n"
		"      break;\n"
		"    case 1:\n"
		"      color = color.rbga;\n"
		"      break;\n"
		"    case 2:\n"
		"      color = color.grba;\n"
		"      break;\n"
		"    case 3:\n"
		"      color = color.brga;\n"
		"      break;\n"
		"    case 4:\n"
		"      color = color.gbra;\n"
		"      break;\n"
		"    case 5:\n"
		"      color = color.bgra;\n"
		"      break;\n"
		"    case 6:\n"
		"      color = color.gbba;\n"
		"      break;\n"
		"    case 7:\n"
		"      color = color.rbba;\n"
		"      break;\n"
		"    case 8:\n"
		"      color = color.rgga;\n"
		"      break;\n"
		"    case 9:\n"
		"      color = color.bbba;\n"
		"      break;\n"
		"    case 10:\n"
		"      color = color.ggga;\n"
		"      break;\n"
		"    case 11:\n"
		"      color = color.rrra;\n"
		"      break;\n"
		"    case 12:\n"
		"      color = color.bbga;\n"
		"      break;\n"
		"    case 13:\n"
		"      color = color.bbra;\n"
		"      break;\n"
		"    case 14:\n"
		"      color = color.ggra;\n"
		"      break;\n"
		"    case 15:\n"
		"      color = color.bgga;\n"
		"      break;\n"
		"    case 16:\n"
		"      color = color.brra;\n"
		"      break;\n"
		"    case 17:\n"
		"      color = color.grra;\n"
		"      break;\n"
		"    case 18:\n"
		"      color = color.bgba;\n"
		"      break;\n"
		"    case 19:\n"
		"      color = color.brba;\n"
		"      break;\n"
		"    case 20:\n"
		"      color = color.grga;\n"
		"      break;\n"
		"    case 21:\n"
		"      color = color.ggba;\n"
		"      break;\n"
		"    case 22:\n"
		"      color = color.rrba;\n"
		"      break;\n"
		"    case 23:\n"
		"      color = color.rrga;\n"
		"      break;\n"
		"    case 24:\n"
		"      color = color.gbga;\n"
		"      break;\n"
		"    case 25:\n"
		"      color = color.rbra;\n"
		"      break;\n"
		"    case 26:\n"
		"      color = color.rgra;\n"
		"      break;\n"
		"    case 27:\n"
		"      color = vec4(color.b, 0.f, 0.f, color.a);\n"
		"      break;\n"
		"    case 28:\n"
		"      color = vec4(0.f, 0.f, 0.f, color.a);\n"
		"      break;\n"
		"  }\n";
	}
	fragmentCodeStream <<
		"  finalColor = color * alpha;\n"
		"}\n";

	static const string fragmentCodeString = fragmentCodeStream.str();
	static const char *fragmentCode = fragmentCodeString.c_str();

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	worldPositionI = shader.Uniform("worldPosition");
	subLightPosI = shader.Uniform("subLightPos");
	subLightColI = shader.Uniform("subLightCol");
//	subLightRadiusI = shader.Uniform("subLightRadius");
	spriteIndexI = shader.Uniform("spriteIndex");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	alphaI = shader.Uniform("alpha");
	if(useShaderSwizzle)
		swizzlerI = shader.Uniform("swizzler");

	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUniform1i(shader.Uniform("normal"), 1);
	glUniform1i(shader.Uniform("base"), 2);
	glUniform1i(shader.Uniform("emit"), 3);
	glUseProgram(0);

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



void ShadowedSpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle, float frame)
{
	if(!sprite)
		return;

	Bind();
	Add(Prepare(sprite, position, zoom, swizzle, frame));
	Unbind();
}



ShadowedSpriteShader::Item ShadowedSpriteShader::Prepare(const Sprite *sprite, const Point &position, float zoom, int swizzle, float frame)
{
	if(!sprite)
		return {};

	Item item;
	item.texture = sprite->Texture();
	item.frame = frame;
	item.frameCount = sprite->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());
	// Rotation (none) and scale.
	item.transform[0] = sprite->Width() * zoom;
	item.transform[3] = sprite->Height() * zoom;
	// Swizzle.
	item.swizzle = swizzle;

	return item;
}



void ShadowedSpriteShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void ShadowedSpriteShader::Add(const Item &item, bool withBlur)
{
	if(item.spriteIndex & 1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, item.texture);
	}
	if(item.spriteIndex & 2)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, item.normal);
	}
	if(item.spriteIndex & 4)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D_ARRAY, item.base);
	}
	if(item.spriteIndex & 8)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D_ARRAY, item.emit);
	}
	glUniform1f(frameI, item.frame);
	glUniform1f(frameCountI, item.frameCount);
	glUniform1i(spriteIndexI, item.spriteIndex);
	glUniform3f(worldPositionI, item.worldPosition[0], item.worldPosition[1], item.worldPosition[2]);
	Light closestLight;
	double shortestDistance = std::numeric_limits<double>::infinity();
	bool hasLight = false;
	Messages::Add(to_string(lights.size()));
	if(!(lights.empty()))
	{

		for(const Light &it : lights)
		{
			if((item.worldSpacePos - it.position).LengthSquared() < shortestDistance)
			{
				closestLight = it;
				shortestDistance = (item.worldSpacePos - it.position).LengthSquared();
				hasLight = true;
			}
		}
		if(hasLight)
		{
			Point offseted = closestLight.position - item.worldSpacePos;
			offseted = (-item.facing).Rotate(-offseted);
			glUniform3f(subLightPosI, -offseted.X(), offseted.Y(), -15.);
			glUniform4fv(subLightColI, 1, closestLight.color.Get());
		//	glUniform1f(subLightRadiusI, closestLight.radius);
		}
	}


	glUniform2fv(positionI, 1, item.position);
	glUniformMatrix2fv(transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
	glUniform1f(clipI, item.clip);
	glUniform1f(alphaI, item.alpha);

	// Bounds check for the swizzle value:
	int swizzle = (static_cast<size_t>(item.swizzle) >= SWIZZLE.size() ? 0 : item.swizzle);
	// Set the color swizzle.
	if(ShadowedSpriteShader::useShaderSwizzle)
		glUniform1i(swizzlerI, swizzle);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SWIZZLE[swizzle].data());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void ShadowedSpriteShader::Unbind()
{
	// Reset the swizzle.
	if(ShadowedSpriteShader::useShaderSwizzle)
		glUniform1i(swizzlerI, 0);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SWIZZLE[0].data());

	// Reset the active texture.
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(0);
	glUseProgram(0);
}
