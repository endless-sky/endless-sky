/* FillShader.cpp
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

#include "FillShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Rectangle.h"
#include "../Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	const Shader *shader;
	GLint scaleI;
	GLint centerI;
	GLint sizeI;
	GLint colorI;

	GLint vertI;

	GLuint vao;
	GLuint vbo;

	void EnableAttribArrays()
	{
		glEnableVertexAttribArray(vertI);
		glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	}
}



void FillShader::Init()
{
	shader = GameData::Shaders().Get("fill");
	if(!shader->Object())
		throw runtime_error("Could not find fill shader!");
	scaleI = shader->Uniform("scale");
	centerI = shader->Uniform("center");
	sizeI = shader->Uniform("size");
	colorI = shader->Uniform("color");
	vertI = shader->Attrib("vert");

	// Generate the vertex data for drawing sprites.
	if(OpenGL::HasVaoSupport())
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		 .5f, -.5f,
		-.5f,  .5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	if(OpenGL::HasVaoSupport())
		EnableAttribArrays();

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



void FillShader::Fill(const Rectangle &area, const Color &color)
{
	Fill(area.Center(), area.Dimensions(), color);
}



void FillShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if(!shader || !shader->Object())
		throw runtime_error("FillShader: Draw() called before Init().");

	glUseProgram(shader->Object());
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(vao);
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		EnableAttribArrays();
	}

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);

	GLfloat centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(centerI, 1, centerV);

	GLfloat sizeV[2] = {static_cast<float>(size.X()), static_cast<float>(size.Y())};
	glUniform2fv(sizeI, 1, sizeV);

	glUniform4fv(colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
	else
	{
		glDisableVertexAttribArray(vertI);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glUseProgram(0);
}
