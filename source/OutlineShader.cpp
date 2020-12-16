/* OutlineShader.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutlineShader.h"

#include "Color.h"
#include "Point.h"
#include "Screen.h"
#include "Shader.h"
#include "Sprite.h"

using namespace std;

namespace {
	Shader shader;
	GLint scaleI;
	GLint offI;
	GLint transformI;
	GLint positionI;
	GLint frameI;
	GLint frameCountI;
	GLint colorI;
	
	GLuint vao;
	GLuint vbo;
}



void OutlineShader::Init()
{
	static const char *vertexCode =
		"// vertex outline shader\n"
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform mat2 transform;\n"
		
		"in vec2 vert;\n"
		"in vec2 vertTexCoord;\n"
		
		"out vec2 fragTexCoord;\n"
		
		"void main() {\n"
		"  fragTexCoord = vertTexCoord;\n"
		"  gl_Position = vec4((transform * vert + position) * scale, 0, 1);\n"
		"}\n";
	
	// The outline shader applies a Sobel filter to an image. The alpha channel
	// (i.e. the silhouette of the sprite) is given the most weight, but some
	// weight is also given to the RGB values so that there will be some detail
	// in the interior of the silhouette as well.
	
	// To reduce sampling error and bring out fine details, for every output
	// pixel the Sobel filter is actually applied in a 3x3 neighborhood and
	// averaged together. That neighborhood's scale is .618034 times the scale
	// of the Sobel neighborhood (i.e. the golden ratio) to minimize any
	// aliasing effects between the two.
	static const char *fragmentCode =
		"// fragment outline shader\n"
		"uniform sampler2DArray tex;\n"
		"uniform float frame = 0;\n"
		"uniform float frameCount = 0;\n"
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		"uniform vec2 off;\n"
		"const vec4 weight = vec4(.4, .4, .4, 1.);\n"
		
		"in vec2 fragTexCoord;\n"
		
		"out vec4 finalColor;\n"
		
		"float Sobel(float layer) {\n"
		"  float sum = 0;\n"
		"  for(int dy = -1; dy <= 1; ++dy)\n"
		"  {\n"
		"    for(int dx = -1; dx <= 1; ++dx)\n"
		"    {\n"
		"      vec2 center = fragTexCoord + .618034 * off * vec2(dx, dy);\n"
		"      float nw = dot(texture(tex, vec3(center + vec2(-off.x, -off.y), layer)), weight);\n"
		"      float ne = dot(texture(tex, vec3(center + vec2(off.x, -off.y), layer)), weight);\n"
		"      float sw = dot(texture(tex, vec3(center + vec2(-off.x, off.y), layer)), weight);\n"
		"      float se = dot(texture(tex, vec3(center + vec2(off.x, off.y), layer)), weight);\n"
		"      float h = nw + sw - ne - se + 2 * (\n"
		"        dot(texture(tex, vec3(center + vec2(-off.x, 0), layer)), weight)\n"
		"          - dot(texture(tex, vec3(center + vec2(off.x, 0), layer)), weight));\n"
		"      float v = nw + ne - sw - se + 2 * (\n"
		"        dot(texture(tex, vec3(center + vec2(0, -off.y), layer)), weight)\n"
		"          - dot(texture(tex, vec3(center + vec2(0, off.y), layer)), weight));\n"
		"      sum += h * h + v * v;\n"
		"    }\n"
		"  }\n"
		"  return sum;\n"
		"}\n"
		
		"void main() {\n"
		"  float first = floor(frame);\n"
		"  float second = mod(ceil(frame), frameCount);\n"
		"  float fade = frame - first;\n"
		"  float sum = mix(Sobel(first), Sobel(second), fade);\n"
		"  finalColor = color * sqrt(sum / 180);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	offI = shader.Uniform("off");
	transformI = shader.Uniform("transform");
	positionI = shader.Uniform("position");
	frameI = shader.Uniform("frame");
	frameCountI = shader.Uniform("frameCount");
	colorI = shader.Uniform("color");
	
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
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
	constexpr auto stride = 4 * sizeof(GLfloat);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, stride, nullptr);
	
	glEnableVertexAttribArray(shader.Attrib("vertTexCoord"));
	glVertexAttribPointer(shader.Attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,
		stride, reinterpret_cast<const GLvoid*>(2 * sizeof(GLfloat)));
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void OutlineShader::Draw(const Sprite *sprite, const Point &pos, const Point &size, const Color &color, const Point &unit, float frame)
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
	
	GLfloat off[2] = {
		static_cast<float>(.5 / size.X()),
		static_cast<float>(.5 / size.Y())};
	glUniform2fv(offI, 1, off);
	
	glUniform1f(frameI, frame);
	glUniform1f(frameCountI, sprite->Frames());
	
	Point uw = unit * size.X();
	Point uh = unit * size.Y();
	GLfloat transform[4] = {
		static_cast<float>(-uw.Y()),
		static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()),
		static_cast<float>(-uh.Y())
	};
	glUniformMatrix2fv(transformI, 1, false, transform);
	
	GLfloat position[2] = {
		static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	glUniform2fv(positionI, 1, position);
	
	glUniform4fv(colorI, 1, color.Get());
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->Texture(unit.Length() * Screen::Zoom() > 50.));
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindVertexArray(0);
	glUseProgram(0);
}
