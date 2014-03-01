/* OutlineShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutlineShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint sizeI;
	GLint positionI;
	
	GLuint vao;
	GLuint vbo;
}



void OutlineShader::Init()
{
	static const char *vertexCode =
		"#version 130\n"
		"uniform vec2 size;\n"
		"uniform vec2 position;\n"
		"uniform vec2 scale;\n"
		"in vec2 vert;\n"
		"in vec2 vertTexCoord;\n"
		"out vec2 tc;\n"
		"out vec2 off;\n"
		"void main() {\n"
		"  tc = vertTexCoord;\n"
		"  off = vec2(.5, .5) / size;\n"
		"  gl_Position = vec4((vert * size + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"#version 130\n"
		"uniform sampler2D tex;\n"
		"in vec2 tc;\n"
		"in vec2 off;\n"
		"out vec4 finalColor;\n"
		"void main() {\n"
		"  float ae = texture(tex, vec2(tc.x - off.x, tc.y)).a;\n"
		"  float aw = texture(tex, vec2(tc.x + off.x, tc.y)).a;\n"
		"  float an = texture(tex, vec2(tc.x, tc.y - off.y)).a;\n"
		"  float as = texture(tex, vec2(tc.x, tc.y + off.y)).a;\n"
		"  float ane = texture(tex, vec2(tc.x - off.x, tc.y - off.y)).a;\n"
		"  float anw = texture(tex, vec2(tc.x + off.x, tc.y - off.y)).a;\n"
		"  float ase = texture(tex, vec2(tc.x - off.x, tc.y + off.y)).a;\n"
		"  float asw = texture(tex, vec2(tc.x + off.x, tc.y + off.y)).a;\n"
		"  float h = (ae * 2 + ane + ase) - (aw * 2 + anw + asw);\n"
		"  float v = (an * 2 + ane + anw) - (as * 2 + ase + asw);\n"
		"  finalColor = vec4(1, 1, 1, 1) * (sqrt(h * h + v * v) * .25);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	sizeI = shader.Uniform("size");
	positionI = shader.Uniform("position");
	
	glUniform1ui(shader.Uniform("tex"), 0);
	
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(GLfloat), NULL);
	
	glEnableVertexAttribArray(shader.Attrib("vertTexCoord"));
	glVertexAttribPointer(shader.Attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,
		4 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void OutlineShader::Draw(const Sprite *sprite, const Point &pos, const Point &size)
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
	
	GLfloat wh[2] = {
		static_cast<float>(size.X()), static_cast<float>(size.Y())};
	glUniform2fv(sizeI, 1, wh);
	
	GLfloat position[2] = {
		static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);
	
	glBindTexture(GL_TEXTURE_2D, sprite->Texture());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindVertexArray(0);
	glUseProgram(0);
}
