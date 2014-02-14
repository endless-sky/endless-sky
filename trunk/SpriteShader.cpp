/* SpriteShader.cpp
Michael Zahniser, 17 Oct 2013

Function definitions for the SpriteShader class.
*/

#include "SpriteShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

#include <cmath>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint transformI;
	GLint positionI;
	GLint clipI;
	GLint fadeI;
	
	GLuint vao;
	GLuint vbo;

	static const GLint SWIZZLE[8][4] = {
		{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA}, // red + yellow markings (republic)
		{GL_RED, GL_BLUE, GL_GREEN, GL_ALPHA}, // red + magenta markings
		{GL_GREEN, GL_RED, GL_BLUE, GL_ALPHA}, // green + yellow (freeholders)
		{GL_BLUE, GL_RED, GL_GREEN, GL_ALPHA}, // green + cyan
		{GL_GREEN, GL_BLUE, GL_RED, GL_ALPHA}, // blue + magenta (syndicate)
		{GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA}, // blue + cyan (merchant)
		{GL_GREEN, GL_BLUE, GL_BLUE, GL_ALPHA}, // red and black (pirate)
		{GL_BLUE, GL_RED, GL_BLUE, GL_ALPHA}  // green only
	};
}



// Initialize the shaders.
void SpriteShader::Init()
{
	static const char *vertexCode =
		"#version 130\n"
		
		"uniform mat2 transform;\t"
		"uniform vec2 position;\t"
		"uniform vec2 scale;\t"
		"uniform float clip;\t"
		
		"in vec2 vert;\n"
		"in vec2 vertTexCoord;\n"
		"out vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  fragTexCoord = vec2(vertTexCoord.x, max(clip, vertTexCoord.y));\n"
		"  gl_Position = vec4((transform * vert + position) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"#version 130\n"
		"uniform sampler2D tex0;\n"
		"uniform sampler2D tex1;\n"
		"uniform float fade;\n"
		
		"in vec2 fragTexCoord;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		// Since fade is a uniform, this branch will not cause performance trouble.
		"  if(fade != 0)\n"
		"    finalColor = mix(texture(tex0, fragTexCoord), texture(tex1, fragTexCoord), fade);\n"
		"  else\n"
		"    finalColor = texture(tex0, fragTexCoord);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	transformI = shader.Uniform("transform");
	positionI = shader.Uniform("position");
	clipI = shader.Uniform("clip");
	fadeI = shader.Uniform("fade");
	
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex0"), 0);
	glUniform1i(shader.Uniform("tex1"), 1);
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



void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom)
{
	if(!sprite)
		return;
	
	float pos[2] = {
		static_cast<float>(position.X()), static_cast<float>(position.Y())};
	float trans[4] = {sprite->Width() * zoom, 0.f, 0.f, sprite->Height() * zoom};
	
	Bind();
	Add(sprite->Texture(), 0, pos, trans, 0, 1.f, 0.f);
	Unbind();
}



void SpriteShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void SpriteShader::Add(uint32_t tex0, uint32_t tex1, const float position[2], const float transform[4], int swizzle, float clip, float fade)
{
	glUniformMatrix2fv(transformI, 1, false, transform);
	glUniform2fv(positionI, 1, position);
	glBindTexture(GL_TEXTURE_2D, tex0);
	
	if(fade && tex1)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glActiveTexture(GL_TEXTURE0);
	}
	else
		fade = 0.f;
	
	// Set the color swizzle.
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, SWIZZLE[swizzle]);
	
	// Set the clipping.
	glUniform1f(clipI, 1.f - clip);
	glUniform1f(fadeI, fade);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
