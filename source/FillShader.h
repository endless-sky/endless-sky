/* FillShader.h
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

#ifndef FILL_SHADER_H_
#define FILL_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "Color.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"

#include <memory>
#include <stdexcept>

// Class holding a function to fill a rectangular region of the screen with a
// given color. This can be used with translucent colors to darken or lighten a
// part of the screen, or with additive colors (alpha = 0) as well.
class FillShader {
private:
	class ShaderState {
	public:
		Shader shader;
		GLint scaleI;
		GLint centerI;
		GLint sizeI;
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
		static void Fill(const Point &center, const Point &size, const Color &color);
	};
public:
	static void Init();
	typedef typename FillShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename FillShader::ShaderImpl<ScaledScreenSpace> UISpace;
};



template<typename T>
FillShader::ShaderState FillShader::ShaderImpl<T>::state;



template<typename T>
void FillShader::ShaderImpl<T>::Init()
{
	static const char *vertexCode =
		"// vertex fill shader\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"

		"in vec2 vert;\n"

		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment fill shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"

		"out vec4 finalColor;\n"

		"void main() {\n"
		"  finalColor = color;\n"
		"}\n";

	state.shader = Shader(vertexCode, fragmentCode);
	state.scaleI = state.shader.Uniform("scale");
	state.centerI = state.shader.Uniform("center");
	state.sizeI = state.shader.Uniform("size");
	state.colorI = state.shader.Uniform("color");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		.5f, -.5f,
		-.5f,  .5f,
		.5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(state.shader.Attrib("vert"));
	glVertexAttribPointer(state.shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



template<typename T>
void FillShader::ShaderImpl<T>::Fill(const Point &center, const Point &size, const Color &color)
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	if(!state.shader.Object())
		throw std::runtime_error("FillShader: Draw() called before Init().");

	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat scale[2] = { 2.f / screenSpace->Width(), -2.f / screenSpace->Height() };
	glUniform2fv(state.scaleI, 1, scale);

	GLfloat centerV[2] = { static_cast<float>(center.X()), static_cast<float>(center.Y()) };
	glUniform2fv(state.centerI, 1, centerV);

	GLfloat sizeV[2] = { static_cast<float>(size.X()), static_cast<float>(size.Y()) };
	glUniform2fv(state.sizeI, 1, sizeV);

	glUniform4fv(state.colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}



#endif
