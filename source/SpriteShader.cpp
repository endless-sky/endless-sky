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

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint transformI;
	GLint positionI;
	GLint blurI;
	GLint clipI;
	GLint fadeI;
	GLint use_bumpI;
	
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



// Initialize the shaders.
void SpriteShader::Init()
{
	static const char *vertexCode = R"code(
		uniform mat2 transform;
		uniform vec2 position;
		uniform vec2 scale;
		uniform vec2 blur;
		uniform float clip;

		in vec2 vert;
		out vec2 fragTexCoord;
		out vec3 lightVec;

		void main() {
			vec2 blurOff = 2 * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));
			gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);
			vec2 texCoord = vert + vec2(.5, .5);
			fragTexCoord = vec2(texCoord.x, max(clip, texCoord.y)) + blurOff;
			vec3 pos3 = vec3(position.x, position.y, 0.0);
			// TODO use direction to actual sun(s)
			lightVec = normalize(vec3(normalize(transform*vec2(1,1)), .6));
		}
	)code";

	static const char *fragmentCode = R"code(
		uniform sampler2D tex0;
		uniform sampler2D tex1;
		uniform sampler2D bump0;
		uniform sampler2D bump1;
		uniform bool use_bump;
		uniform float fade;
		uniform vec2 blur;
		const int range = 5;

		in vec2 fragTexCoord;
		in vec3 lightVec;
		out vec4 finalColor;

		vec4 colorAt(const vec2 coord, const sampler2D tex, const sampler2D bump) {
			vec4 color = texture(tex, coord);
			float scale = 1;
			if (use_bump) {
				vec3 normal = texture(bump, coord).xyz*2-1;
				scale = dot(normal, lightVec);
			}
			color.xyz *= scale;
			return color;
		}

		vec4 colorAt(const vec2 coord) {
			vec4 color = colorAt(coord, tex0, bump0);
			if (fade != 0)
				color = mix(color, colorAt(coord, tex1, bump1), fade);
			return color;
		}

		void main() {
			if(blur.x == 0 && blur.y == 0)
			{
				finalColor = colorAt(fragTexCoord);
				return;
			}
			const float divisor = range * (range + 2) + 1;
			vec4 color = vec4(0., 0., 0., 0.);
			for(int i = -range; i <= range; ++i)
			{
				float scale = (range + 1 - abs(i)) / divisor;
				vec2 coord = fragTexCoord + (blur * i) / range;
				color += scale*colorAt(coord);
			}
			finalColor = color;
		}
	)code";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	transformI = shader.Uniform("transform");
	positionI = shader.Uniform("position");
	blurI = shader.Uniform("blur");
	clipI = shader.Uniform("clip");
	fadeI = shader.Uniform("fade");
	use_bumpI = shader.Uniform("use_bump");
	
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex0"), 0);
	glUniform1i(shader.Uniform("tex1"), 1);
	glUniform1i(shader.Uniform("bump0"), 2);
	glUniform1i(shader.Uniform("bump1"), 3);
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



void SpriteShader::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle)
{
	if(!sprite)
		return;
	
	float pos[2] = {
		static_cast<float>(position.X()), static_cast<float>(position.Y())};
	float trans[4] = {sprite->Width() * zoom, 0.f, 0.f, sprite->Height() * zoom};
	
	Bind();
	Add(sprite->Texture(), 0, sprite->Bumpmap(), 0, pos, trans, swizzle, 1.f, 0.f);
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



void SpriteShader::Add(uint32_t tex0, uint32_t tex1, uint32_t bump0, uint32_t bump1, const float position[2], const float transform[4], int swizzle, float clip, float fade, const float blur[2])
{
	glUniformMatrix2fv(transformI, 1, false, transform);
	glUniform2fv(positionI, 1, position);
	glBindTexture(GL_TEXTURE_2D, tex0);
	
	// Bounds check for the swizzle value:
	if(static_cast<size_t>(swizzle) >= SWIZZLE.size())
		swizzle = 0;
	const GLint *swizzleValues = SWIZZLE[swizzle].data();
	
	if(fade && tex1)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleValues);
		glActiveTexture(GL_TEXTURE0);
	}
	else
		fade = 0.f;
	
	if (bump0)
	{
		glUniform1i(use_bumpI, true);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, bump0);
		// Swizzling the bumpmap would be hilarious.
		if(fade && bump1)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, bump1);
		}
		glActiveTexture(GL_TEXTURE0);
	} else
		glUniform1i(use_bumpI, 0);

	// Set the color swizzle.
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleValues);
	
	// Set the clipping.
	glUniform1f(clipI, 1.f - clip);
	glUniform1f(fadeI, fade);
	const float noBlur[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, blur ? blur : noBlur);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}
