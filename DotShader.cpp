/* DotShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "DotShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"

#include <stdexcept>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint positionI;
	GLint outRadiusI;
	GLint inRadiusI;
	GLint colorI;
	
	GLuint vao;
	GLuint vbo;
}



void DotShader::Init()
{
	static const char *vertexCode =
#ifdef __APPLE__
		"#version 330\n"
#else
		"#version 130\n"
#endif
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform float outRadius;\n"
		
		"in vec2 vert;\n"
		"out vec2 coord;\n"
		
		"void main() {\n"
		"  coord = (outRadius + 1) * vert;\n"
		"  gl_Position = vec4((coord + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
#ifdef __APPLE__
		"#version 330\n"
#else
		"#version 130\n"
#endif
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		"uniform float outRadius;\n"
		"uniform float inRadius;\n"
		
		"in vec2 coord;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  float len = length(coord);\n"
		"  float alpha = clamp(outRadius - len, 0, 1);\n"
		"  alpha *= clamp(len - inRadius, -1, 0) + 1;\n"
		"  finalColor = color * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	positionI = shader.Uniform("position");
	outRadiusI = shader.Uniform("outRadius");
	inRadiusI = shader.Uniform("inRadius");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertexData[] = {
		-1.f, -1.f,
		 0.f,  0.f,
		-1.f,  1.f,
		 1.f,  1.f,
		 1.f,  1.f,
		 1.f, -1.f,
		 0.f,  0.f,
		-1.f, -1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE,
		2 * sizeof(GLfloat), NULL);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void DotShader::Draw(const Point &pos, float out, float in, const Color &color)
{
	Bind();
	
	Add(pos, out, in, color);
	
	Unbind();
}



void DotShader::Bind()
{
	if(!shader.Object())
		throw runtime_error("DotShader: Bind() called before Init().");
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void DotShader::Add(const Point &pos, float out, float in, const Color &color)
{
	GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);
	
	glUniform1f(outRadiusI, out);
	glUniform1f(inRadiusI, in);
	
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
}



void DotShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
