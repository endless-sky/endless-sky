/* ShipFXShader.cpp
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

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

#include <sstream>
#include <vector>
#include "Logger.h"

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

	GLint recentHitsCountI;
	GLfloat recentDamageI;
	GLfloat recentHitsI;

	GLuint vao;
	GLuint vbo;
}

// Initialize the shaders.
void ShipFXShader::Init()
{

	static const char* vertexCode =
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
		"uniform float frame;\n"
		"uniform float frameCount;\n"
		"uniform vec2 blur;\n"
		"uniform float alpha;\n"
		"const int range = 5;\n"

		"uniform vec2 recentHits[32];\n"
		"uniform float recentDamage[32];\n"
		"uniform int recentHitCount;\n"

		"in vec2 fragTexCoord;\n"
		"in vec2 shrinkby;\n"

		"out vec4 finalColor;\n"

		"float first = floor(frame);\n"
	"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first + second;\n";

		//"void main() {\n"
		//"  float first = floor(frame);\n"
		//"  float second = mod(ceil(frame), frameCount);\n"
		//"  float fade = frame - first;\n"
		//"  vec4 color;\n"
		//"  if(blur.x == 0.f && blur.y == 0.f)\n"
		//"  {\n"
		//"    if(fade != 0.f)\n"
		//"      color = mix(\n"
		//"        texture(tex, vec3(fragTexCoord, first)),\n"
		//"        texture(tex, vec3(fragTexCoord, second)), fade);\n"
		//"    else\n"
		//"      color = texture(tex, vec3(fragTexCoord, first));\n"
		//"  }\n"
		//"  else\n"
		//"  {\n"
		//"    color = vec4(0., 0., 0., 0.);\n"
		//"    const float divisor = float(range * (range + 2) + 1);\n"
		//"    for(int i = -range; i <= range; ++i)\n"
		//"    {\n"
		//"      float scale = float(range + 1 - abs(i)) / divisor;\n"
		//"      vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
		//"      if(fade != 0.f)\n"
		//"        color += scale * mix(\n"
		//"          texture(tex, vec3(coord, first)),\n"
		//"          texture(tex, vec3(coord, second)), fade);\n"
		//"      else\n"
		//"        color += scale * texture(tex, vec3(coord, first));\n"
		//"    }\n"
		//"  }\n"

		//"  for(int i = 0; i < recentHitCount && i < 32; i++)\n"
		//"  {\n"
		//"    color -= clamp(distance(fragTexCoord, recentHits[i] * shrinkby), 0, 1);\n"
		//"  }\n"

		//"  finalColor = color * alpha;\n"
		fragmentCodeStream <<
		"vec4 sampleSmooth(sampler2DArray sampler, vec2 uv)\n"
		"{\n"
			"return mix( texture(tex, vec3(uv, first)),\n"
			"			texture(tex, vec3(uv, second)), fade);}\n"	
		"float sobellish(vec2 uv)\n"
		"{\n"
		"			float obel = 0.;\n"
		"			for (int x = -3; x <= 3; x++)\n"
		"			{\n"
		"				for (int y = -3; y <= 3; y++)\n"
		"				{\n"
		"					obel += sampleSmooth(tex, uv + vec2(x, y) / (100.)).a;\n"
		"				}\n"
		"			}\n"
		"			obel /= 49.;\n"
		"			return sqrt(2. * obel - (-0.2) / (obel / 2. - .6) + 0.3);\n"
		"		}\n"

"		void main()\n"
"		{\n"
"vec4 addCol;"
		"  for(int i = 0; i < recentHitCount; i++)\n"
		"  {\n"
"				vec2 hitPoint = recentHits[i];\n"
""
//"				/* Normalized pixel coordinates (from 0 to 1)*/\n"
//"				vec4 col = sampleSmooth(tex, fragTexCoord);\n"
"				vec4 color = vec4(0.3, 0.4, 1., 1.) * sobellish(fragTexCoord).rrrr;\n"
""
"			    addCol += (color*10.*recentDamage[i]) * clamp(1. - distance(fragTexCoord, hitPoint) / 2, 0., 1.) * 10.; \n"
"			}"
"			/* Output to screen*/"
"			finalColor = (addCol / recentHitCount) * alpha;\n"
			//"finalColor += alpha * vec4(sobellish(fragTexCoord));\n"
"		}\n"
"	    \n";

	static const string fragmentCodeString = fragmentCodeStream.str();
	static const char *fragmentCode = fragmentCodeString.c_str();

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	alphaI = shader.Uniform("alpha");

	recentHitsI = shader.Uniform("recentHits");
	recentDamageI = shader.Uniform("recentDamage");
	recentHitsCountI = shader.Uniform("recentHitCount");

	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
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



void ShipFXShader::Draw(const Body* body, const Point& position, std::vector<pair<Point, double>>& recentHits, float zoom, float frame)
{
	if (!body->GetSprite())
		return;

	Bind();
	Add(Prepare(body, position, recentHits, zoom, frame));
	Unbind();
}



ShipFXShader::EffectItem ShipFXShader::Prepare(const Body* body, const Point& position, vector<pair<Point, double>>& recentHits, float zoom, float frame)
{
	if (!body->GetSprite())
		return {};

	EffectItem item;
	item.texture = body->GetSprite()->Texture();
	item.frame = frame;
	item.frameCount = body->GetSprite()->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());

	// Get unit vectors in the direction of the object's width and height.
	double width = body->Width();
	double height = body->Height();
	Point unit = body->Facing().Unit();
	Point uw = unit * width;
	Point uh = unit * height;

	// (0, -1) means a zero-degree rotation (since negative Y is up).
	uw *= zoom;
	uh *= zoom;
	item.transform[0] = -uw.Y();
	item.transform[1] = uw.X();
	item.transform[2] = -uh.X();
	item.transform[3] = -uh.Y();

	item.recentHitPoints.clear();
	auto recth = recentHits;
	item.recentHitPoints.reserve(recth.size() * 2);
	for(int i = 0; i < recth.size(); i++)
	{
			const auto newP = body->Facing().Rotate(recth[i].first);
			item.recentHitPoints.push_back(newP.X() / (2. * body->Radius()));
			item.recentHitPoints.push_back(newP.Y() / (2. * body->Radius()));
			item.recentHitDamage.push_back(recth[i].second);
			Logger::LogError("Hit at " + to_string(newP.X() / body->Radius()) + ", " + to_string(newP.Y() / body->Radius()) + ", intensity of " + to_string(recth[i].second));
	}
	/*for (pair<Point, double>& hit : recentHits)
	{
		item.recentHitPoints.push_back(hit.first.X());
		item.recentHitPoints.push_back(hit.first.Y());
	}*/
	item.recentHits = recth.size();
	Logger::LogError("RECENT_HIT_SIZE" + to_string(item.recentHits));

	return item;
}



void ShipFXShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = { 2.f / Screen::Width(), -2.f / Screen::Height() };
	glUniform2fv(scaleI, 1, scale);
}



void ShipFXShader::Add(const EffectItem& item, bool withBlur)
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
	glUniform1f(alphaI, item.alpha);

	glUniform2fv(recentHitsI, 32, item.recentHitPoints.data());
	glUniform1fv(recentDamageI, 32, item.recentHitDamage.data());
	glUniform1i(recentHitsCountI, min(item.recentHits, 32));
	Logger::LogError(to_string(item.recentHits));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void ShipFXShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
