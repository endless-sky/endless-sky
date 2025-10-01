/* PolygonShader.cpp
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

#include "PolygonShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	const int N = 7;

	const Shader *shader;
	GLint scaleI;
	GLint insideColorI;
	GLint borderColorI;
	GLint borderWidthI;
	GLint numSidesI;
	GLint polygonI;

	GLuint vao;
	GLuint vbo;
}



void PolygonShader::Init()
{
	shader = GameData::Shaders().Get("polygon");
	if(!shader->Object())
		throw std::runtime_error("Could not find polygon shader!");

	insideColorI = shader->Uniform("insideColor");
	borderColorI = shader->Uniform("borderColor");
	borderWidthI = shader->Uniform("borderWidth");
	numSidesI = shader->Uniform("numSides");
	polygonI = shader->Uniform("polygon");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-1.f, -1.f,
		-1.f,  1.f,
		 1.f, -1.f,
		 1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader->Attrib("vert"));
	glVertexAttribPointer(shader->Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void PolygonShader::Draw(const vector<Point> &polygon, const Color &insideColor, const Color &borderColor,
	double borderWidth)
{
	if(!shader || !shader->Object())
		throw runtime_error("PolygonShader: Draw() called before Init().");

	glUseProgram(shader->Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);

	GLfloat positions[N][2];
	int i = 0;
	for(auto point: polygon)
	{
		positions[i][0] = static_cast<GLfloat>(point.X());
		positions[i][1] = static_cast<GLfloat>(point.Y());
		i++;
	}

	glUniform2fv(polygonI, i, *positions);
	glUniform1i(numSidesI, i);

	glUniform1i(borderWidthI, borderWidth);

	glUniform4fv(insideColorI, 1, insideColor.Get());
	glUniform4fv(borderColorI, 1, borderColor.Opaque().Get());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
