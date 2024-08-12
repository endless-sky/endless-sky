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

	GLuint vao;
	GLuint vbo;
}



void LineShader::Init()
{
	// static const char *vertexCode =
	// 	"// vertex line shader\n"
	// 	"uniform vec2 scale;\n"
	// 	"uniform vec2 start;\n"
	// 	"uniform vec2 len;\n"
	// 	"uniform vec2 width;\n"

	// 	"in vec2 vert;\n"
	// 	"out vec2 tpos;\n"
	// 	"out float tscale;\n"

	// 	"void main() {\n"
	// 	"  tpos = vert;\n"
	// 	"  tscale = length(len);\n"
	// 	"  gl_Position = vec4((start + vert.x * len + vert.y * width) * scale, 0, 1);\n"
	// 	"}\n";

	// static const char *fragmentCode =
	// 	"// fragment line shader\n"
	// 	"precision mediump float;\n"
	// 	"uniform vec4 color;\n"

	// 	"in vec2 tpos;\n"
	// 	"in float tscale;\n"
	// 	"out vec4 finalColor;\n"

	// 	"void main() {\n"
	// 	"  float alpha = min(tscale - abs(tpos.x * (2.f * tscale) - tscale), 1.f - abs(tpos.y));\n"
	// 	"  finalColor = color * alpha;\n"
	// 	"}\n";
	static const char *vertexCode = R"(
// vertex line shader

uniform vec2 scale;

uniform vec2 start;
uniform vec2 end;
uniform float width;

in vec2 vert;
out vec2 pos;

void main() {
    vec2 unit = normalize(end - start);
    vec2 origin = vert.y > 0.0 ? start : end;
    pos = origin + vec2(unit.y, -unit.x) * vert.x * width - unit * width * vert.y;
    gl_Position = vec4(pos / scale, 0, 1);
    gl_Position.y = -gl_Position.y;
    gl_Position.xy *= 2.0;
}
)";

	static const char *fragmentCode = R"(
// fragment line shader
precision mediump float;

uniform vec2 start;
uniform vec2 end;
uniform float width;
uniform vec4 color;

in vec2 pos;
out vec4 finalColor;

float udSegment(vec2 p, vec2 a, vec2 b) {
    vec2 ba = b-a;
    vec2 pa = p-a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0);
    return length(pa-h*ba);
}
float sdOrientedBox(vec2 p, vec2 a, vec2 b, float th) {
    float l = length(b-a);
    vec2  d = (b-a)/l;
    vec2  q = (p-(a+b)*0.5);
          q = mat2(d.x,-d.y,d.y,d.x)*q;
          q = abs(q)-vec2(l,th)*0.5;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}
void main() {
    float alpha = clamp(1.0 - sdOrientedBox(pos, start, end, width), 0.0, 1.0);
    finalColor = color * alpha;
}
)";

	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	startI = shader.Uniform("start");
	endI = shader.Uniform("end");
	widthI = shader.Uniform("width");
	colorI = shader.Uniform("color");

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



void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color)
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

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}



void LineShader::DrawDashed(const Point &from, const Point &to, const Point &unit, const float width,
		const Color &color, const double dashLength, double spaceLength)
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
	for(int i = 0; i < segments; ++i)
		Draw(from + unit * ((i * length) / segments + spaceLength),
			from + unit * (((i + 1) * length) / segments - spaceLength),
			width, color);
}
