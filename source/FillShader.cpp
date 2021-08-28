/* FillShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FillShader.h"

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
	GLint sizeI;
	GLint colorI;
	
	GLuint vao;
	GLuint vbo;
}



void FillShader::Init()
{
	static const char *vertexCode =
		"// vertex fill shader\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"
		
		"in vec2 vert;\n"
		
		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment fill shader\n"
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  finalColor = color;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	sizeI = shader.Uniform("size");
	colorI = shader.Uniform("color");
	
	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertexData[] = {
		-.5f, -.5f,
		 .5f, -.5f,
		-.5f,  .5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void FillShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if(!shader.Object())
		throw runtime_error("FillShader: Draw() called before Init().");
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
	
	GLfloat centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	glUniform2fv(centerI, 1, centerV);
	
	GLfloat sizeV[2] = {static_cast<float>(size.X()), static_cast<float>(size.Y())};
	glUniform2fv(sizeI, 1, sizeV);
	
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindVertexArray(0);
	glUseProgram(0);
}
