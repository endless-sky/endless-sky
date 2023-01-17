/* PointerShader.h
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

#ifndef POINTER_SHADER_H_
#define POINTER_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "Color.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"

#include <memory>
#include <stdexcept>

using namespace std;

// Functions for drawing triangular "pointers," e.g. for target crosshairs.
class PointerShader {
private:
	class ShaderState {
	public:
		Shader shader;
		GLint scaleI;
		GLint centerI;
		GLint angleI;
		GLint sizeI;
		GLint offsetI;
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

		static void Draw(const Point &center, const Point &angle, float width, float height, float offset, const Color &color);

		static void Bind();
		static void Add(const Point &center, const Point &angle, float width, float height, float offset, const Color &color);
		static void Unbind();
	};
public:
	static void Init();

	typedef typename PointerShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename PointerShader::ShaderImpl<ScaledScreenSpace> UISpace;
};

template<typename T>
PointerShader::ShaderState PointerShader::ShaderImpl<T>::state;

template<typename T>
void PointerShader::ShaderImpl<T>::Init()
{
	static const char *vertexCode =
		"// vertex pointer shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 angle;\n"
		"uniform vec2 size;\n"
		"uniform float offset;\n"

		"in vec2 vert;\n"
		"out vec2 coord;\n"

		"void main() {\n"
		"  coord = vert * size.x;\n"
		"  vec2 base = center + angle * (offset - size.y * (vert.x + vert.y));\n"
		"  vec2 wing = vec2(angle.y, -angle.x) * (size.x * .5 * (vert.x - vert.y));\n"
		"  gl_Position = vec4((base + wing) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment pointer shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"uniform vec2 size;\n"

		"in vec2 coord;\n"
		"out vec4 finalColor;\n"

		"void main() {\n"
		"  float height = (coord.x + coord.y) / size.x;\n"
		"  float taper = height * height * height;\n"
		"  taper *= taper * .5 * size.x;\n"
		"  float alpha = clamp(.8 * min(coord.x, coord.y) - taper, 0.f, 1.f);\n"
		"  alpha *= clamp(1.8 * (1. - height), 0.f, 1.f);\n"
		"  finalColor = color * alpha;\n"
		"}\n";

	state.shader = Shader(vertexCode, fragmentCode);
	state.scaleI = state.shader.Uniform("scale");
	state.centerI = state.shader.Uniform("center");
	state.angleI = state.shader.Uniform("angle");
	state.sizeI = state.shader.Uniform("size");
	state.offsetI = state.shader.Uniform("offset");
	state.colorI = state.shader.Uniform("color");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	GLfloat vertexData[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(state.shader.Attrib("vert"));
	glVertexAttribPointer(state.shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


template<typename T>
void PointerShader::ShaderImpl<T>::Draw(const Point &center, const Point &angle,
	float width, float height, float offset, const Color &color)
{
	Bind();

	Add(center, angle, width, height, offset, color);

	Unbind();
}


template<typename T>
void PointerShader::ShaderImpl<T>::Bind()
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!state.shader.Object())
		throw runtime_error("PointerShader: Bind() called before Init().");

	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat scale[2] = {2.f / screenSpace->Width(), -2.f / screenSpace->Height()};
	glUniform2fv(state.scaleI, 1, scale);
}


template<typename T>
void PointerShader::ShaderImpl<T>::Add(const Point &center, const Point &angle,
	float width, float height, float offset, const Color &color)
{
	GLfloat c[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(state.centerI, 1, c);

	GLfloat a[2] = {static_cast<float>(angle.X()), static_cast<float>(angle.Y())};
	glUniform2fv(state.angleI, 1, a);

	GLfloat size[2] = {width, height};
	glUniform2fv(state.sizeI, 1, size);

	glUniform1f(state.offsetI, offset);

	glUniform4fv(state.colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLES, 0, 3);
}


template<typename T>
void PointerShader::ShaderImpl<T>::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}


#endif
