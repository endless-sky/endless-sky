/* BatchShader.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "BatchShader.h"

#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

using namespace std;

namespace {
	Shader shader;
	// Uniforms:
	GLint scaleI;
	GLint frameCountI;
	// Vertex data:
	GLint vertI;
	GLint texCoordI;
	
	GLuint vao;
	GLuint vbo;
}



// Initialize the shaders.
void BatchShader::Init()
{
	static const char *vertexCode =
		"// vertex batch shader\n"
		"uniform vec2 scale;\n"
		"in vec2 vert;\n"
		"in vec3 texCoord;\n"
		
		"out vec3 fragTexCoord;\n"
		
		"void main() {\n"
		"  gl_Position = vec4(vert * scale, 0, 1);\n"
		"  fragTexCoord = texCoord;\n"
		"}\n";
	
	static const char *fragmentCode =
		"// fragment batch shader\n"
		"uniform sampler2DArray tex;\n"
		"uniform float frameCount;\n"
		
		"in vec3 fragTexCoord;\n"
		
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  float first = floor(fragTexCoord.z);\n"
		"  float second = mod(ceil(fragTexCoord.z), frameCount);\n"
		"  float fade = fragTexCoord.z - first;\n"
		"  finalColor = mix(\n"
		"    texture(tex, vec3(fragTexCoord.xy, first)),\n"
		"    texture(tex, vec3(fragTexCoord.xy, second)), fade);\n"
		"}\n";
	
	// Compile the shaders.
	shader = Shader(vertexCode, fragmentCode);
	// Get the indices of the uniforms and attributes.
	scaleI = shader.Uniform("scale");
	frameCountI = shader.Uniform("frameCount");
	vertI = shader.Attrib("vert");
	texCoordI = shader.Attrib("texCoord");
	
	// Make sure we're using texture 0.
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);
	
	// Generate the buffer for uploading the batch vertex data.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// In this VAO, enable the two vertex arrays and specify their byte offsets.
	constexpr auto stride = 5 * sizeof(float);
	glEnableVertexAttribArray(vertI);
	glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
	// The 3 texture fields (s, t, frame) come after the x,y pixel fields.
	auto textureOffset = reinterpret_cast<const GLvoid *>(2 * sizeof(float));
	glEnableVertexAttribArray(texCoordI);
	glVertexAttribPointer(texCoordI, 3, GL_FLOAT, GL_FALSE, stride, textureOffset);
	
	// Unbind the buffer and the VAO, but leave the vertex attrib arrays enabled
	// in the VAO so they will be used when it is bound.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void BatchShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	// Bind the vertex buffer so we can upload data to it.
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// Set up the screen scale.
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void BatchShader::Add(const Sprite *sprite, bool isHighDPI, const vector<float> &data)
{
	// Do nothing if there are no sprites to draw.
	if(data.empty())
		return;
	
	// First, bind the proper texture.
	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->Texture(isHighDPI));
	// The shader also needs to know how many frames the texture has.
	glUniform1f(frameCountI, sprite->Frames());
	
	// Upload the vertex data.
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), data.data(), GL_STREAM_DRAW);
	
	// Draw all the vertices.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, data.size() / 5);
}



void BatchShader::Unbind()
{
	// Unbind everything in reverse order.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
