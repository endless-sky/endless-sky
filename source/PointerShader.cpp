/* PointerShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PointerShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint centerI;
	GLint angleI;
	GLint sizeI;
	GLint offsetI;
	GLint colorI;
	
	GLuint vao;
	GLuint vbo;
}



void PointerShader::Init()
{
	static const char *vertexCode =
		"// vertex pointer shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 angle;\n"
		"uniform vec2 size;\n"
		"uniform float offset;\n"
		
		"in vec2 vert;\n"
		"out vec2 coord;\n"
		
		"void main() {\n"
		"  coord = vert * size.x;\n"
		"  vec2 base = center + angle * (offset - size.y * (vert.x + vert.y));\n"
		"  vec2 wing = vec2(angle.y, -angle.x) * (size.x * .5 * (vert.x - vert.y));\n"
		"  gl_Position = vec4((base + wing) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment pointer shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"uniform vec2 size;\n"
		
		"in vec2 coord;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  float height = (coord.x + coord.y) / size.x;\n"
		"  float taper = height * height * height;\n"
		"  taper *= taper * .5 * size.x;\n"
		"  float alpha = clamp(.8 * min(coord.x, coord.y) - taper, 0.f, 1.f);\n"
		"  alpha *= clamp(1.8 * (1. - height), 0.f, 1.f);\n"
		"  finalColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	angleI = shader.Uniform("angle");
	sizeI = shader.Uniform("size");
	offsetI = shader.Uniform("offset");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertexData[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void PointerShader::Draw(const Point &center, const Point &angle, float width, float height, float offset, const Color &color)
{
	Bind();
	
	Add(center, angle, width, height, offset, color);
	
	Unbind();
}


	
void PointerShader::Bind()
{
	if(!shader.Object())
		throw runtime_error("PointerShader: Bind() called before Init().");
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void PointerShader::Add(const Point &center, const Point &angle, float width, float height, float offset, const Color &color)
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
	glBindVertexArray(0);
	glUseProgram(0);
}
