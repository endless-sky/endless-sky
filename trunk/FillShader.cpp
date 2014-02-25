/* FillShader.cpp
Michael Zahniser, 14 Dec 2013

Function definitions for the FillShader class.
*/

#include "FillShader.h"

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
		"#version 130\n"
		"uniform vec2 scale;\n"
		"uniform vec2 center;\n"
		"uniform vec2 size;\n"
		
		"in vec2 vert;\n"
		
		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"#version 130\n"
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		
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
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE,
		2 * sizeof(GLfloat), NULL);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void FillShader::Fill(const Point &center, const Point &size, const float *color)
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
	
	static const float white[4] = {1.f, 1.f, 1.f, 1.f};
	glUniform4fv(colorI, 1, color ? color : white);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindVertexArray(0);
	glUseProgram(0);
}
