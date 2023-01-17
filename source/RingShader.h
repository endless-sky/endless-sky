/* RingShader.h
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

#ifndef RING_SHADER_H_
#define RING_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "Color.h"
#include "pi.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"

#include <memory>
#include <stdexcept>

using namespace std;

// Class representing a state.shader that draws round "dots," either filled in or with
// transparent centers (i.e. circles or rings).
class RingShader
{
private:
	class ShaderState
	{
	public:
		Shader shader;
		GLint scaleI;
		GLint positionI;
		GLint radiusI;
		GLint widthI;
		GLint angleI;
		GLint startAngleI;
		GLint dashI;
		GLint colorI;

		GLuint vao;
		GLuint vbo;
	};
	template <typename T>
	class ShaderImpl
	{
	private:
		static ShaderState state;

	public:
		static void Draw(const Point &pos, float out, float in, const Color &color);
		static void Draw(const Point &pos, float radius, float width, float fraction,
						const Color &color, float dash = 0.f, float startAngle = 0.f);

		static void Bind();
		static void Add(const Point &pos, float out, float in, const Color &color);
		static void Add(const Point &pos, float radius, float width, float fraction,
						const Color &color, float dash = 0.f, float startAngle = 0.f);
		static void Unbind();
		static void Init();
	};

public:
	static void Init();
	typedef typename RingShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename RingShader::ShaderImpl<ScaledScreenSpace> UISpace;
};

#endif

template <typename T>
RingShader::ShaderState RingShader::ShaderImpl<T>::state;

template <typename T>
void RingShader::ShaderImpl<T>::Init()
{
	static const char *vertexCode =
		"// vertex ring state.shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform float radius;\n"
		"uniform float width;\n"

		"in vec2 vert;\n"
		"out vec2 coord;\n"

		"void main() {\n"
		"  coord = (radius + width) * vert;\n"
		"  gl_Position = vec4((coord + position) * scale, 0.f, 1.f);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment ring state.shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"uniform float radius;\n"
		"uniform float width;\n"
		"uniform float angle;\n"
		"uniform float startAngle;\n"
		"uniform float dash;\n"
		"const float pi = 3.1415926535897932384626433832795;\n"

		"in vec2 coord;\n"
		"out vec4 finalColor;\n"

		"void main() {\n"
		"  float arc = mod(atan(coord.x, coord.y) + pi + startAngle, 2.f * pi);\n"
		"  float arcFalloff = 1.f - min(2.f * pi - arc, arc - angle) * radius;\n"
		"  if(dash != 0.f)\n"
		"  {\n"
		"    arc = mod(arc, dash);\n"
		"    arcFalloff = min(arcFalloff, min(arc, dash - arc) * radius);\n"
		"  }\n"
		"  float len = length(coord);\n"
		"  float lenFalloff = width - abs(len - radius);\n"
		"  float alpha = clamp(min(arcFalloff, lenFalloff), 0.f, 1.f);\n"
		"  finalColor = color * alpha;\n"
		"}\n";

	state.shader = Shader(vertexCode, fragmentCode);
	state.scaleI = state.shader.Uniform("scale");
	state.positionI = state.shader.Uniform("position");
	state.radiusI = state.shader.Uniform("radius");
	state.widthI = state.shader.Uniform("width");
	state.angleI = state.shader.Uniform("angle");
	state.startAngleI = state.shader.Uniform("startAngle");
	state.dashI = state.shader.Uniform("dash");
	state.colorI = state.shader.Uniform("color");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	GLfloat vertexData[] = {
		-1.f, -1.f,
		-1.f, 1.f,
		1.f, -1.f,
		1.f, 1.f};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(state.shader.Attrib("vert"));
	glVertexAttribPointer(state.shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

template <typename T>
void RingShader::ShaderImpl<T>::Draw(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in);
	Draw(pos, out - width, width, 1.f, color);
}

template <typename T>
void RingShader::ShaderImpl<T>::Draw(const Point &pos, float radius, float width, float fraction,
									const Color &color, float dash, float startAngle)
{
	Bind();

	Add(pos, radius, width, fraction, color, dash, startAngle);

	Unbind();
}

template <typename T>
void RingShader::ShaderImpl<T>::Bind()
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!state.shader.Object())
		throw runtime_error("RingShader: Bind() called before Init().");

	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat scale[2] = {2.f / screenSpace->Width(), -2.f / screenSpace->Height()};
	glUniform2fv(state.scaleI, 1, scale);
}

template <typename T>
void RingShader::ShaderImpl<T>::Add(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in);
	Add(pos, out - width, width, 1.f, color);
}

template <typename T>
void RingShader::ShaderImpl<T>::Add(const Point &pos, float radius, float width, float fraction,
									const Color &color, float dash, float startAngle)
{
	GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(state.positionI, 1, position);

	glUniform1f(state.radiusI, radius);
	glUniform1f(state.widthI, width);
	glUniform1f(state.angleI, fraction * 2. * PI);
	glUniform1f(state.startAngleI, startAngle * TO_RAD);
	glUniform1f(state.dashI, dash ? 2. * PI / dash : 0.);

	glUniform4fv(state.colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

template <typename T>
void RingShader::ShaderImpl<T>::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
