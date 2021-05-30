/* SpriteShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "SpriteShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

#include <vector>
#include <sstream>

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint frameI;
	GLint frameCountI;
	GLint positionI;
	GLint transformI;
	GLint blurI;
	GLint clipI;
	GLint alphaI;
	GLint swizzlerI;
	
	GLuint vao;
	GLuint vbo;

	const vector<vector<GLint>> SWIZZLE = {
		{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA}, // red + yellow markings (republic)
		{GL_RED, GL_BLUE, GL_GREEN, GL_ALPHA}, // red + magenta markings
		{GL_GREEN, GL_RED, GL_BLUE, GL_ALPHA}, // green + yellow (freeholders)
		{GL_BLUE, GL_RED, GL_GREEN, GL_ALPHA}, // green + cyan
		{GL_GREEN, GL_BLUE, GL_RED, GL_ALPHA}, // blue + magenta (syndicate)
		{GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA}, // blue + cyan (merchant)
		{GL_GREEN, GL_BLUE, GL_BLUE, GL_ALPHA}, // red and black (pirate)
		{GL_BLUE, GL_ZERO, GL_ZERO, GL_ALPHA},  // red only (cloaked)
		{GL_ZERO, GL_ZERO, GL_ZERO, GL_ALPHA}  // black only (outline)
	};
}

bool SpriteShader::useShaderSwizzle = false;

// Initialize the shaders.
void SpriteShader::Init(bool useShaderSwizzle)
{
	SpriteShader::useShaderSwizzle = useShaderSwizzle;
	
	static const char *vertexCode =
		"// vertex sprite shader\n"
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform mat2 transform;\n"
		"uniform vec2 blur;\n"
		"uniform float clip;\n"
		
		"in vec2 vert;\n"
		"out vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  vec2 blurOff = 2 * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
		"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);\n"
		"  vec2 texCoord = vert + vec2(.5, .5);\n"
		"  fragTexCoord = vec2(texCoord.x, max(clip, texCoord.y)) + blurOff;\n"
		"}\n";
	
	ostringstream fragmentCodeStream;
	fragmentCodeStream <<
		"// fragment sprite shader\n"
		"uniform sampler2DArray tex;\n"
		"uniform float frame;\n"
		"uniform float frameCount;\n"
		"uniform vec2 blur;\n";
	if(useShaderSwizzle) fragmentCodeStream <<
		"uniform int swizzler;\n";
	fragmentCodeStream <<
		"uniform float alpha;\n"
		"const int range = 5;\n"
		
		"in vec2 fragTexCoord;\n"
		
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  float first = floor(frame);\n"
		"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first;\n"
		"  vec4 color;\n"
		"  if(blur.x == 0 && blur.y == 0)\n"
		"  {\n"
		"    if(fade != 0)\n"
		"      color = mix(\n"
		"        texture(tex, vec3(fragTexCoord, first)),\n"
		"        texture(tex, vec3(fragTexCoord, second)), fade);\n"
		"    else\n"
		"      color = texture(tex, vec3(fragTexCoord, first));\n"
		"  }\n"
		"  else\n"
		"  {\n"
		"    color = vec4(0., 0., 0., 0.);\n"
		"    const float divisor = range * (range + 2) + 1;\n"
		"    for(int i = -range; i <= range; ++i)\n"
		"    {\n"
		"      float scale = (range + 1 - abs(i)) / divisor;\n"
		"      vec2 coord = fragTexCoord + (blur * i) / range;\n"
		"      if(fade != 0)\n"
		"        color += scale * mix(\n"
		"          texture(tex, vec3(coord, first)),\n"
		"          texture(tex, vec3(coord, second)), fade);\n"
		"      else\n"
		"        color += scale * texture(tex, vec3(coord, first));\n"
		"    }\n"
		"  }\n";
	
	// Only included when hardware swizzle not supported, GL <3.3 and GLES
	if(useShaderSwizzle)
	{
		fragmentCodeStream <<
		"  switch (swizzler) {\n"
		"    case 0:\n"
		"      color = color.rgba;\n"
		"      break;\n"
		"    case 1:\n"
		"      color = color.rbga;\n"
		"      break;\n"
		"    case 2:\n"
		"      color = color.grba;\n"
		"      break;\n"
		"    case 3:\n"
		"      color = color.brga;\n"
		"      break;\n"
		"    case 4:\n"
		"      color = color.gbra;\n"
		"      break;\n"
		"    case 5:\n"
		"      color = color.bgra;\n"
		"      break;\n"
		"    case 6:\n"
		"      color = color.gbba;\n"
		"      break;\n"
		"    case 7:\n"
		"      color = vec4(color.b, 0.f, 0.f, color.a);\n"
		"      break;\n"
		"    case 8:\n"
		"      color = vec4(0.f, 0.f, 0.f, color.a);\n"
		"      break;\n"
		"  }\n";
	}
	fragmentCodeStream <<
		"  finalColor = color * alpha;\n"
		"}\n";
	
	static const string fragmentCodeString = fragmentCodeStream.str();
	static const char *fragmentCode = fragmentCodeString.c_str();
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	positionI = shader.Uniform("position");
	transformI = shader.Uniform("transform");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	alphaI = shader.Uniform("alpha");
	if(useShaderSwizzle)
		swizzlerI = shader.Uniform("swizzler");
	
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);
	
	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle, float frame)
{
	if(!sprite)
		return;
	
	Item item;
	item.texture = sprite->Texture();
	item.frame = frame;
	item.frameCount = sprite->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());
	// Rotation (none) and scale.
	item.transform[0] = sprite->Width() * zoom;
	item.transform[3] = sprite->Height() * zoom;
	// Swizzle.
	item.swizzle = swizzle;
	
	Bind();
	Add(item);
	Unbind();
}



void SpriteShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void SpriteShader::Add(const Item &item, bool withBlur)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, item.texture);

	glUniform1f(frameI, item.frame);
	glUniform1f(frameCountI, item.frameCount);
	glUniform2fv(positionI, 1, item.position);
	glUniformMatrix2fv(transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
	// Clipping has the opposite sense in the shader.
	glUniform1f(clipI, 1.f - item.clip);
	glUniform1f(alphaI, item.alpha);
	
	// Bounds check for the swizzle value:
	int swizzle = (static_cast<size_t>(item.swizzle) >= SWIZZLE.size() ? 0 : item.swizzle);
	// Set the color swizzle.
	if(SpriteShader::useShaderSwizzle)
		glUniform1i(swizzlerI, swizzle);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SWIZZLE[swizzle].data());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	// Reset the swizzle.
	if(SpriteShader::useShaderSwizzle)
		glUniform1i(swizzlerI, 0);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SWIZZLE[0].data());

	glBindVertexArray(0);
	glUseProgram(0);
}
