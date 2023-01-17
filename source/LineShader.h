/* LineShader.h
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

#ifndef LINE_SHADER_H_
#define LINE_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "Color.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"

#include <memory>
#include <stdexcept>

using namespace std;

// Class to be used for drawing lines. The sides of a line are anti-aliased, but
// the start and end of the line are not.
class LineShader {
private:
	class ShaderState {
	public:
		Shader shader;
		GLint scaleI;
		GLint startI;
		GLint lengthI;
		GLint widthI;
		GLint colorI;

		GLuint vao;
		GLuint vbo;
	};

	template<typename T>
	class ShaderImpl {
	private:
		static ShaderState state;
	public:
		static void Init();
		static void Draw(const Point &from, const Point &to, float width, const Color &color);
	};
public:
	static void Init();
	typedef typename LineShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename LineShader::ShaderImpl<ScaledScreenSpace> UISpace;
};

template<typename T>
LineShader::ShaderState LineShader::ShaderImpl<T>::state;

template<typename T>
void LineShader::ShaderImpl<T>::Init()
{
	static const char *vertexCode =
		"// vertex line shader\n"
		"uniform vec2 scale;\n"
		"uniform vec2 start;\n"
		"uniform vec2 len;\n"
		"uniform vec2 width;\n"

		"in vec2 vert;\n"
		"out vec2 tpos;\n"
		"out float tscale;\n"

		"void main() {\n"
		"  tpos = vert;\n"
		"  tscale = length(len);\n"
		"  gl_Position = vec4((start + vert.x * len + vert.y * width) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment line shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"

		"in vec2 tpos;\n"
		"in float tscale;\n"
		"out vec4 finalColor;\n"

		"void main() {\n"
		"  float alpha = min(tscale - abs(tpos.x * (2.f * tscale) - tscale), 1.f - abs(tpos.y));\n"
		"  finalColor = color * alpha;\n"
		"}\n";

	state.shader = Shader(vertexCode, fragmentCode);
	state.scaleI = state.shader.Uniform("scale");
	state.startI = state.shader.Uniform("start");
	state.lengthI = state.shader.Uniform("len");
	state.widthI = state.shader.Uniform("width");
	state.colorI = state.shader.Uniform("color");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	GLfloat vertexData[] = {
		0.f, -1.f,
		1.f, -1.f,
		0.f,  1.f,
		1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(state.shader.Attrib("vert"));
	glVertexAttribPointer(state.shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


template<typename T>
void LineShader::ShaderImpl<T>::Draw(const Point &from, const Point &to, float width, const Color &color)
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!state.shader.Object())
		throw runtime_error("LineShader: Draw() called before Init().");

	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat scale[2] = {2.f / screenSpace->Width(), -2.f / screenSpace->Height()};
	glUniform2fv(state.scaleI, 1, scale);

	GLfloat start[2] = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	glUniform2fv(state.startI, 1, start);

	Point v = to - from;
	Point u = v.Unit() * width;
	GLfloat length[2] = {static_cast<float>(v.X()), static_cast<float>(v.Y())};
	glUniform2fv(state.lengthI, 1, length);

	GLfloat w[2] = {static_cast<float>(u.Y()), static_cast<float>(-u.X())};
	glUniform2fv(state.widthI, 1, w);

	glUniform4fv(state.colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}

#endif
