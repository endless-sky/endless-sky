/* UiRectShader.cpp
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "UiRectShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "shader/Shader.h"

#include <stdexcept>
#include <string>

namespace {
	Shader shader;
	GLint scaleI;
	GLint centerI;
	GLint sizeI;
	GLint colorI;

	GLuint vao;
	GLuint vbo;
}



void UiRectShader::Init(const Color& border1, const Color& border2, const Color& border3)
{
	static const char *vertexCode =
		"// vertex uirect shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"

		"in vec2 vert;\n"
		"out float borderGradient;\n"
		"out vec2 screenCoords;\n"

		"void main() {\n"
		"  screenCoords = center + vert * size;\n"
		"  gl_Position = vec4(screenCoords * scale, 0, 1);\n"
		"  borderGradient = vert.x - vert.y;\n"
		"}\n";
	// TODO: If you use an odd size, then your center coordinate needs to be
	//       the center of a pixel (ie, 10.5, 13.5, etc). otherwise it tries
	//       to smear the border across both pixels, which looks like garbage.
	static const std::string fragmentCode =
		"// fragment uirect shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"const vec4 bg1 = vec4("+std::to_string(border1.Get()[0])+", "+std::to_string(border1.Get()[1])+", "+std::to_string(border1.Get()[2])+", 1.0);\n"
		"const vec4 bg2 = vec4("+std::to_string(border2.Get()[0])+", "+std::to_string(border2.Get()[1])+", "+std::to_string(border2.Get()[2])+", 1.0);\n"
		"const vec4 bg3 = vec4("+std::to_string(border3.Get()[0])+", "+std::to_string(border3.Get()[1])+", "+std::to_string(border3.Get()[2])+", 1.0);\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"

		"out vec4 finalColor;\n"
		"in float borderGradient;\n"
		"in vec2 screenCoords;\n"


		"void main() {\n"
		"  if (screenCoords.x - 1.0 > center.x - size.x / 2.0 && \n"
		"      screenCoords.y - 1.0 > center.y - size.y / 2.0 && \n"
		"      screenCoords.y + 1.0 < center.y + size.y / 2.0 && \n"
		"      screenCoords.x + 1.0 < center.x + size.x / 2.0)\n"
		"    finalColor = color;\n"
		"  else\n"
		"  {\n"
		"    if (borderGradient < 0.0)\n"
		"      finalColor = mix(bg2, bg1, -borderGradient);\n"
		"    else\n"
		"      finalColor = mix(bg2, bg3, borderGradient);\n"
		"  }\n"
		"}\n";

	shader = Shader(vertexCode, fragmentCode.c_str());
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	sizeI = shader.Uniform("size");
	colorI = shader.Uniform("color");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		 .5f, -.5f,
		-.5f,  .5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void UiRectShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if(!shader.Object())
		throw std::runtime_error("UiRectShader: Draw() called before Init().");

	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);

	GLfloat centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(centerI, 1, centerV);

	GLfloat sizeV[2] = {static_cast<float>(size.X()), static_cast<float>(size.Y())};
	glUniform2fv(sizeI, 1, sizeV);

	glUniform4fv(colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}
