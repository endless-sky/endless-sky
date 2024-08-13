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

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint startI;
	GLint endI;
	GLint widthI;
	GLint colorI;
	GLint capI;

	GLuint vao;
	GLuint vbo;
}



void LineShader::Init()
{
	static const char *vertexCode =
		"// vertex line shader\n"
		"\n"
		"uniform vec2 scale;\n"
		"\n"
		"uniform vec2 start;\n"
		"uniform vec2 end;\n"
		"uniform float width;\n"
		"uniform int cap;\n"
		"\n"
		"in vec2 vert;\n"
		"out vec2 pos;\n"
		"\n"
		"void main() {\n"
		"    vec2 unit = normalize(end - start);\n"
		"    vec2 origin = vert.y > 0.0 ? start : end;\n"
		"    float widthOffset = width + 1;\n"
		"    pos = origin + vec2(unit.y, -unit.x) * vert.x * widthOffset - unit * (cap == 1 ? widthOffset : 1) * vert.y;\n"
		"    gl_Position = vec4(pos / scale, 0, 1);\n"
		"    gl_Position.y = -gl_Position.y;\n"
		"    gl_Position.xy *= 2.0;\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment line shader\n"
		"precision mediump float;\n"
		"\n"
		"uniform vec2 start;\n"
		"uniform vec2 end;\n"
		"uniform float width;\n"
		"uniform vec4 color;\n"
		"uniform int cap;\n"
		"\n"
		"in vec2 pos;\n"
		"out vec4 finalColor;\n"
		"\n"
		"float udSegment(vec2 p, vec2 a, vec2 b) {\n"
		"    vec2 ba = b-a;\n"
		"    vec2 pa = p-a;\n"
		"    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0);\n"
		"    return length(pa-h*ba);\n"
		"}\n"
		"\n"
		"float sdOrientedBox(vec2 p, vec2 a, vec2 b, float th) {\n"
		"    float l = length(b-a);\n"
		"    vec2  d = (b-a)/l;\n"
		"    vec2  q = (p-(a+b)*0.5);\n"
		"          q = mat2(d.x,-d.y,d.y,d.x)*q;\n"
		"          q = abs(q)-vec2(l,th)*0.5;\n"
		"    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);\n"
		"}\n"
		"\n"
		"void main() {\n"
		"    float dist;\n"
		"    if(cap == 1) {\n"
		"        dist = width - udSegment(pos, start, end);\n"
		"    } else {\n"
		"        dist = 1. - sdOrientedBox(pos, start, end, width);\n"
		"    }\n"
		"    float alpha = clamp(dist, 0.0, 1.0);\n"
		"    finalColor = color * alpha;\n"
		"}\n";

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	startI = shader.Uniform("start");
	endI = shader.Uniform("end");
	widthI = shader.Uniform("width");
	colorI = shader.Uniform("color");
	capI = shader.Uniform("cap");

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-1.f, -1.f,
		1.f, -1.f,
		-1.f,  1.f,
		1.f,  1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color, Cap capKind)
{
	if(!shader.Object())
		throw runtime_error("LineShader: Draw() called before Init().");

	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {static_cast<GLfloat>(Screen::Width()), static_cast<GLfloat>(Screen::Height())};
	glUniform2fv(scaleI, 1, scale);

	GLfloat start[2] = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	glUniform2fv(startI, 1, start);

	GLfloat end[2] = {static_cast<float>(to.X()), static_cast<float>(to.Y())};
	glUniform2fv(endI, 1, end);

	// GLfloat w[2] = {static_cast<float>(u.Y()), static_cast<float>(-u.X())};
	glUniform1f(widthI, width);

	glUniform4fv(colorI, 1, color.Get());

	glUniform1i(capI, static_cast<GLint>(capKind));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}



void LineShader::DrawDashed(const Point &from, const Point &to, const Point &unit, const float width,
		const Color &color, const double dashLength, double spaceLength, Cap capKind)
{
	const double length = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int segments = static_cast<int>(length / patternLength);
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	float capOffset = capKind == Cap::Rounded ? width : 0.;
	for(int i = 0; i < segments; ++i)
		Draw(from + unit * ((i * length) / segments + spaceLength + capOffset),
			from + unit * (((i + 1) * length) / segments - spaceLength - capOffset),
			width, color, capKind);
}
