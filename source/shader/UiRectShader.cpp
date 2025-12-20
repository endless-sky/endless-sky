/* UiRectShader.cpp
Copyright (c) 2025 by thewierdnut

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

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Screen.h"
#include "Shader.h"

#include <stdexcept>
#include <string>

namespace {
	const Shader *shader = nullptr;
	GLint scaleI = -1;
	GLint centerI = -1;
	GLint sizeI = -1;
	GLint colorI = -1;
   GLint bg1I = -1;
   GLint bg2I = -1;
   GLint bg3I = -1;

	GLuint vao = -1;
	GLuint vbo = -1;

   Color bg1{};
   Color bg2{};
   Color bg3{};
}



void UiRectShader::Init()
{
	shader = GameData::Shaders().Get("uirect");
	scaleI = shader->Uniform("scale");
	centerI = shader->Uniform("center");
	sizeI = shader->Uniform("size");
	colorI = shader->Uniform("color");
   bg1I = shader->Uniform("bg1");
   bg2I = shader->Uniform("bg2");
   bg3I = shader->Uniform("bg3");

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

	glEnableVertexAttribArray(shader->Attrib("vert"));
	glVertexAttribPointer(shader->Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void UiRectShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if(!shader->Object())
		throw std::runtime_error("UiRectShader: Draw() called before Init().");

   if(bg1 == Color() && bg2 == Color() && bg3 == Color())
   {
      bg1 = GameData::Colors().Get("medium")->Transparent(1);
      bg2 = GameData::Colors().Get("dim")->Transparent(1);
      bg3 = GameData::Colors().Get("bright")->Transparent(1);
   }

	glUseProgram(shader->Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);

	GLfloat centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(centerI, 1, centerV);

	GLfloat sizeV[2] = {static_cast<float>(size.X()), static_cast<float>(size.Y())};
	glUniform2fv(sizeI, 1, sizeV);

	glUniform4fv(colorI, 1, color.Get());
	glUniform4fv(bg1I, 1, bg1.Get());
	glUniform4fv(bg2I, 1, bg2.Get());
	glUniform4fv(bg3I, 1, bg3.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}
