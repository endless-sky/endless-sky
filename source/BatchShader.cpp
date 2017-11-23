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
}



// Initialize the shaders.
void BatchShader::Init()
{
	static const char *vertexCode =
		"uniform vec2 scale;\n"
		"attribute vec2 vert;\n"
		"attribute vec3 texCoord;\n"
		
		"out vec3 fragTexCoord;\n"
		
		"void main() {\n"
		"  gl_Position = vec4(vert * scale, 0, 1);\n"
		"  fragTexCoord = texCoord;\n"
		"}\n";
	
	static const char *fragmentCode =
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
}



void BatchShader::Bind()
{
	glUseProgram(shader.Object());
	
	// Set up the screen scale.
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void BatchShader::Add(const Sprite *sprite, bool isHighDPI, const vector<float> &data)
{
	// First, bind the proper texture.
	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->Texture(isHighDPI));
	// The shader also needs to know how many frames the texture has.
	glUniform1f(frameCountI, sprite->Frames());
	
	// Specify the vertex data.
	glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), data.data());
	glEnableVertexAttribArray(vertI);
	glVertexAttribPointer(texCoordI, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), data.data() + 2);
	glEnableVertexAttribArray(texCoordI);
	
	// Draw all the vertices.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, data.size() / 5);
	
	// Disable the vertex buffers.
	glDisableVertexAttribArray(texCoordI);
	glDisableVertexAttribArray(vertI);
}



void BatchShader::Unbind()
{
	glUseProgram(0);
}
