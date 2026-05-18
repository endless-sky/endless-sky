/* RingShader.cpp
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

#include "RingShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../pi.h"
#include "../Point.h"
#include "../Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	const Shader *shader;
	GLint scaleI;
	GLint positionI;
	GLint radiusI;
	GLint widthI;
	GLint angleI;
	GLint startAngleI;
	GLint dashI;
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



void RingShader::Init()
{
	shader = GameData::Shaders().Get("ring");
	if(!shader->Object())
		throw runtime_error("Could not find ring shader!");
	scaleI = shader->Uniform("scale");
	positionI = shader->Uniform("position");
	radiusI = shader->Uniform("radius");
	widthI = shader->Uniform("width");
	angleI = shader->Uniform("angle");
	startAngleI = shader->Uniform("startAngle");
	dashI = shader->Uniform("dash");
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
		-1.f, -1.f,
		-1.f,  1.f,
		 1.f, -1.f,
		 1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	if(OpenGL::HasVaoSupport())
		EnableAttribArrays();

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



void RingShader::Draw(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Draw(pos, out - width, width, 1.f, color);
}



void RingShader::Draw(const Point &pos, float radius, float width, float fraction,
	const Color &color, float dash, float startAngle)
{
	Bind();

	Add(pos, radius, width, fraction, color, dash, startAngle);

	Unbind();
}



void RingShader::Bind()
{
	if(!shader || !shader->Object())
		throw runtime_error("RingShader: Bind() called before Init().");

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



void RingShader::Add(const Point &pos, float out, float in, const Color &color)
{
	float width = .5f * (1.f + out - in) ;
	Add(pos, out - width, width, 1.f, color);
}



void RingShader::Add(const Point &pos, float radius, float width, float fraction,
	const Color &color, float dash, float startAngle)
{
	GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);

	glUniform1f(radiusI, radius);
	glUniform1f(widthI, width);
	glUniform1f(angleI, fraction * 2. * PI);
	glUniform1f(startAngleI, startAngle * TO_RAD);
	glUniform1f(dashI, dash ? 2. * PI / dash : 0.);

	glUniform4fv(colorI, 1, color.Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void RingShader::Unbind()
{
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
	else
	{
		glDisableVertexAttribArray(vertI);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
	glUseProgram(0);
}
