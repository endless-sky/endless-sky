/* PointerShader.cpp
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

#include "PointerShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	const Shader *shader;
	GLint scaleI;
	GLint centerI;
	GLint angleI;
	GLint sizeI;
	GLint offsetI;
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



void PointerShader::Init()
{
	shader = GameData::Shaders().Get("pointer");
	if(!shader->Object())
		throw runtime_error("Could not find pointer shader!");
	scaleI = shader->Uniform("scale");
	centerI = shader->Uniform("center");
	angleI = shader->Uniform("angle");
	sizeI = shader->Uniform("size");
	offsetI = shader->Uniform("offset");
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
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	if(OpenGL::HasVaoSupport())
		EnableAttribArrays();

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



void PointerShader::Draw(const Point &center, const Point &angle,
	float width, float height, float offset, const Color &color)
{
	Bind();

	Add(center, angle, width, height, offset, color);

	Unbind();
}



void PointerShader::Bind()
{
	if(!shader || !shader->Object())
		throw runtime_error("PointerShader: Bind() called before Init().");

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
}



void PointerShader::Add(const Point &center, const Point &angle,
	float width, float height, float offset, const Color &color)
{
	GLfloat c[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(centerI, 1, c);

	GLfloat a[2] = {static_cast<float>(angle.X()), static_cast<float>(angle.Y())};
	glUniform2fv(angleI, 1, a);

	GLfloat size[2] = {width, height};
	glUniform2fv(sizeI, 1, size);

	glUniform1f(offsetI, offset);

	glUniform4fv(colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLES, 0, 3);
}



void PointerShader::Unbind()
{
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
	else
	{
		glDisableVertexAttribArray(vertI);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glUseProgram(0);
}
