/* OutlineShader.cpp
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

#include "OutlineShader.h"

#include "Color.h"
#include "GameData.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "image/Sprite.h"

using namespace std;

namespace {
	const Shader *shader;
	GLint scaleI;
	GLint offI;
	GLint transformI;
	GLint positionI;
	GLint frameI;
	GLint frameCountI;
	GLint colorI;

	GLuint vao;
	GLuint vbo;
}



void OutlineShader::Init()
{
	shader = GameData::Shaders().Get("outline");
	scaleI = shader->Uniform("scale");
	offI = shader->Uniform("off");
	transformI = shader->Uniform("transform");
	positionI = shader->Uniform("position");
	frameI = shader->Uniform("frame");
	frameCountI = shader->Uniform("frameCount");
	colorI = shader->Uniform("color");

	glUseProgram(shader->Object());
	glUniform1i(shader->Uniform("tex"), 0);
	glUseProgram(0);

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f, 0.f, 0.f,
		 .5f, -.5f, 1.f, 0.f,
		-.5f,  .5f, 0.f, 1.f,
		 .5f,  .5f, 1.f, 1.f
	};
	constexpr auto stride = 4 * sizeof(GLfloat);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(shader->Attrib("vert"));
	glVertexAttribPointer(shader->Attrib("vert"), 2, GL_FLOAT, GL_FALSE, stride, nullptr);

	glEnableVertexAttribArray(shader->Attrib("vertTexCoord"));
	glVertexAttribPointer(shader->Attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,
		stride, reinterpret_cast<const GLvoid*>(2 * sizeof(GLfloat)));

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void OutlineShader::Draw(const Sprite *sprite, const Point &pos, const Point &size,
	const Color &color, const Point &unit, float frame)
{
	glUseProgram(shader->Object());
	glBindVertexArray(vao);

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);

	GLfloat off[2] = {
		static_cast<float>(.5 / size.X()),
		static_cast<float>(.5 / size.Y())};
	glUniform2fv(offI, 1, off);

	glUniform1f(frameI, frame);
	glUniform1f(frameCountI, sprite->Frames());

	Point uw = unit * size.X();
	Point uh = unit * size.Y();
	GLfloat transform[4] = {
		static_cast<float>(-uw.Y()),
		static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()),
		static_cast<float>(-uh.Y())
	};
	glUniformMatrix2fv(transformI, 1, false, transform);

	GLfloat position[2] = {
		static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);

	glUniform4fv(colorI, 1, color.Get());

	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->Texture(unit.Length() * Screen::Zoom() > 50.));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}
