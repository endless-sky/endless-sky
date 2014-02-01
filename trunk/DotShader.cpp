/* DotShader.cpp
Michael Zahniser, 31 Oct 2013

Function definitions for the DotShader class.
*/

#include "DotShader.h"

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
		"#version 130\n"
		"uniform vec2 scale;\t"
		"uniform vec2 position;\t"
		"uniform float outRadius;\t"
		
		"in vec2 vert;\n"
		"out vec2 coord;\n"
		
		"void main() {\n"
		"  coord = (outRadius + 1) * vert;\n"
		"  gl_Position = vec4((coord + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"#version 130\n"
		"uniform vec4 color = vec4(1, 1, 1, 1);\t"
		"uniform float outRadius;\t"
		"uniform float inRadius;\t"
		
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



void DotShader::Draw(const Point &pos, float out, float in, const float *color)
{
	Bind();
	
	static const float white[4] = {1.f, 1.f, 1.f, 1.f};
	Add(pos, out, in, color ? color : white);
	
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



void DotShader::Add(const Point &pos, float out, float in, const float *color)
{
	GLfloat position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);
	
	glUniform1f(outRadiusI, out);
	glUniform1f(inRadiusI, in);
	
	glUniform4fv(colorI, 1, color);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
}



void DotShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
