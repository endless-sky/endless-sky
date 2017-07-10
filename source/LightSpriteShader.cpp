/* LightSpriteShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "LightSpriteShader.h"

#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

#include <stdexcept>
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
	GLint transformGSI;
	GLint posGSI;
	GLint nbLightI;
	GLint lightPosI;
	GLint lightEmitI;
	GLint lightAmbiantI;
	GLint angCoeffI;
	GLint selfLightI;
	
	GLuint vao;
	GLuint vbo;
	
	bool isAvailable=false;

	static const vector<vector<GLint>> SWIZZLE = {
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

const float LightSpriteShader::DEF_AMBIENT[3] = {0.5f, 0.5f, 0.5f};

// Initialize the shaders.
void LightSpriteShader::Init()
{
	try{
		static const char *vertexCode =
			"uniform mat2 transform;\n"
			"uniform vec2 position;\n"
			"uniform vec2 scale;\n"
			"uniform vec2 blur;\n"
			"uniform float clip;\n"
			"uniform vec2 posGS;\n"
			"uniform mat2 transformGS;\n"
		
			"in vec2 vert;\n"
			"out vec2 fragTexCoord;\n"
			"out vec2 gameSpacePos;\n"
		
			"void main() {\n"
			"  vec2 blurOff = 2 * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
			"  gameSpacePos = transformGS *vert + posGS;\n" 
			"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);\n"
			"  vec2 texCoord = vert + vec2(.5, .5);\n"
			"  fragTexCoord = vec2(texCoord.x, max(clip, texCoord.y)) + blurOff;\n"
			"}\n";

		static const char *fragmentCode =
			"uniform sampler2D tex0;\n"
			"uniform sampler2D tex1;\n"
			"uniform sampler2D texL;\n"
			"uniform float fade;\n"
			"uniform float selfLight;\n"
			"uniform vec2 blur;\n"
			"uniform mat2 transformGS;\n"
			"uniform int nbLight;\n"
			"uniform vec3 lightPos[5];\n"
			"uniform vec3 lightEmit[5];\n"
			"uniform vec3 lightAmbiant;\n" 
			"uniform float angCoeff;\n"
			"const int range = 5;\n"
		
			"in vec2 fragTexCoord;\n"
			"in vec2 gameSpacePos;"
			"out vec4 finalColor;\n"
		
			"void main() {\n"
			"  vec4 color = vec4(0., 0., 0., 0.);\n"
			"  if(false && blur.x == 0 && blur.y == 0)\n"
			"  {\n"
			"    if(fade != 0)\n"
			"      color = mix(texture(tex0, fragTexCoord), texture(tex1, fragTexCoord), fade);\n"
			"    else\n"
			"      color = texture(tex0, fragTexCoord);\n"
			"  } else {\n"
			"    const float divisor = range * (range + 2) + 1;\n"
			"    for(int i = -range; i <= range; ++i)\n"
			"    {\n"
			"      float scale = (range + 1 - abs(i)) / divisor;\n"
			"      vec2 coord = fragTexCoord + (blur * i) / range;\n"
			"      if(fade != 0)\n"
			"        color += scale * mix(texture(tex0, coord), texture(tex1, coord), fade);\n"
			"      else\n"
			"        color += scale * texture(tex0, coord);\n"
			"    }\n"
			"  }\n"
			"  if(nbLight < 0){\n"
			"    finalColor = color;\n"
			"    return;\n"
			"  }\n"
			"  vec3 lightColor = lightAmbiant;\n"
			"  vec3 normal = 2 * vec3( transformGS*(fragTexCoord-vec2(.5,.5)) ,0);\n"
			"  float xy2 = pow(normal.x,2) + pow(normal.y,2);\n"
			"  if(xy2>1) normal = normalize(normal);\n"
			"  else normal.z = sqrt(1-xy2);\n"
			"  for(int i=0;i<nbLight;i++){\n"
			"    vec3 lightDir = lightPos[i]-vec3(gameSpacePos,0);\n"
			"    float dst = length(lightDir);\n"
			"    lightColor += (1-angCoeff+angCoeff*dot(lightDir,normal)/dst) * lightEmit[i] / pow(dst,2);\n"
			"  }\n"
			"  if(selfLight != 0)\n"
			"    lightColor += selfLight * texture(texL, fragTexCoord).xyz;\n"
			"  lightColor = clamp(lightColor, vec3(0.3,0.3,0.3), vec3(1,1,1));\n"
			"  finalColor = vec4(lightColor*color.xyz,color.w);\n"
			"}\n";
	
		shader = Shader(vertexCode, fragmentCode);
		scaleI = shader.Uniform("scale");
		transformI = shader.Uniform("transform");
		positionI = shader.Uniform("position");
		blurI = shader.Uniform("blur");
		clipI = shader.Uniform("clip");
		fadeI = shader.Uniform("fade");
		transformGSI = shader.Uniform("transformGS");
		posGSI = shader.Uniform("posGS");
		nbLightI = shader.Uniform("nbLight");
		lightPosI = shader.Uniform("lightPos");
		lightEmitI = shader.Uniform("lightEmit");
		lightAmbiantI = shader.Uniform("lightAmbiant");
		angCoeffI = shader.Uniform("angCoeff");
		selfLightI = shader.Uniform("selfLight");
	
		glUseProgram(shader.Object());
		glUniform1i(shader.Uniform("tex0"), 0);
		glUniform1i(shader.Uniform("tex1"), 1);
		glUniform1i(shader.Uniform("texL"), 2);
		glUseProgram(0);
	} catch(runtime_error& e){
		isAvailable = false;
		return;
	}
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
	
	isAvailable = true;
}


void LightSpriteShader::Bind()
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}


void LightSpriteShader::Add(uint32_t tex0, uint32_t tex1, const float position[2], const float transform[4], int swizzle, float clip, float fade, const float blur[2], const float posGS[2], const float transformGS[2], int nbLight, const float lightAmbiant[3], const float *lightPos, const float *lightEmit, float angCoeff, float selfLight, uint32_t texL)
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
	
	// Set the color swizzle.
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleValues);
	
	// Set the clipping.
	glUniform1f(clipI, 1.f - clip);
	glUniform1f(fadeI, fade);
	const float noBlur[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, blur ? blur : noBlur);

	glUniform2fv(posGSI,1,posGS);
	glUniformMatrix2fv(transformGSI, 1, false, transformGS);
	glUniform1i(nbLightI, nbLight);
	if(nbLight > 0)
	{
		glUniform3fv(lightPosI, nbLight, lightPos);
		glUniform3fv(lightEmitI, nbLight, lightEmit);
	}
	glUniform3fv(lightAmbiantI, 1, lightAmbiant);
	glUniform1f(angCoeffI, angCoeff);

	if(selfLight && texL)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texL);
	}
	else
		selfLight = 0.f;
	glUniform1f(selfLightI, selfLight);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void LightSpriteShader::Unbind()
{
	glBindVertexArray(0);
	glUseProgram(0);
}

bool LightSpriteShader::IsAvailable()
{
	return isAvailable;
}

int LightSpriteShader::MaxNbLights()
{
	return 5;
}
