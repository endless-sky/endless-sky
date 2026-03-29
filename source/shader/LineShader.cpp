/* LineShader.cpp
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

#include "LineShader.h"

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
	GLint startI;
	GLint endI;
	GLint widthI;
	GLint fromColorI;
	GLint toColorI;
	GLint capI;

	GLint vertI;

	GLuint vao;
	GLuint vbo;

	void EnableAttribArrays()
	{
		glEnableVertexAttribArray(vertI);
		glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	}
}



void LineShader::Init()
{
	shader = GameData::Shaders().Get("line");
	if(!shader->Object())
		throw runtime_error("Could not find line shader!");
	scaleI = shader->Uniform("scale");
	startI = shader->Uniform("start");
	endI = shader->Uniform("end");
	widthI = shader->Uniform("width");
	fromColorI = shader->Uniform("startColor");
	toColorI = shader->Uniform("endColor");
	capI = shader->Uniform("cap");
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
		1.f, -1.f,
		-1.f,  1.f,
		1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	EnableAttribArrays();

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color, bool roundCap)
{
	DrawGradient(from, to, width, color, color, roundCap);
}



void LineShader::DrawDashed(const Point &from, const Point &to, const Point &unit, const float width,
	const Color &color, const double dashLength, double spaceLength, bool roundCap)
{
	const double length = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int segments = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
		Draw(from + unit * (i * length / segments + spaceLength + capOffset),
			from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
			width, color, roundCap);
}



void LineShader::DrawGradient(const Point &from, const Point &to, float width,
	const Color &fromColor, const Color &toColor, bool roundCap)
{
	if(!shader->Object())
		throw runtime_error("LineShader: Draw() called before Init().");

	glUseProgram(shader->Object());
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(vao);
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		EnableAttribArrays();
	}

	GLfloat scale[2] = {static_cast<GLfloat>(Screen::Width()), static_cast<GLfloat>(Screen::Height())};
	glUniform2fv(scaleI, 1, scale);

	GLfloat start[2] = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	glUniform2fv(startI, 1, start);

	GLfloat end[2] = {static_cast<float>(to.X()), static_cast<float>(to.Y())};
	glUniform2fv(endI, 1, end);

	glUniform1f(widthI, width);

	glUniform4fv(fromColorI, 1, fromColor.Get());
	glUniform4fv(toColorI, 1, toColor.Get());

	glUniform1i(capI, static_cast<GLint>(roundCap));

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



void LineShader::DrawGradientDashed(const Point &from, const Point &to, const Point &unit, const float width,
		const Color &fromColor, const Color &toColor, const double dashLength, double spaceLength, bool roundCap)
{
	const double length = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int segments = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
	{
		float p = static_cast<float>(i) / segments;
		Color mixed = Color::Combine(1. - p, fromColor, p, toColor);
		float pv = static_cast<float>(i + 1) / segments;
		Color mixed2 = Color::Combine(1. - pv, fromColor, pv, toColor);
		DrawGradient(from + unit * (i * length / segments + spaceLength + capOffset),
			from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
			width, mixed, mixed2, roundCap);
	}
}
