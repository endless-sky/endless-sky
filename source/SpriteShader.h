/* SpriteShader.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SPRITE_SHADER_H_
#define SPRITE_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"
#include "Sprite.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

class DrawList;

using namespace std;

#ifdef ES_GLES
// ES_GLES always uses the shader, not this, so use a dummy value to compile.
// (the correct value is usually 0x8E46, so don't use that)
#define GL_TEXTURE_SWIZZLE_RGBA 0xBEEF
#endif

// Class for drawing sprites. You can optionally draw a sprite with a custom
// zoom level or color swizzle. A more complicated function is also provided for
// adjusting the scale, rotation, clipping, fading, etc. of a sprite; this is
// most often just for use by the DrawList class, which calculates those input
// parameters based on an object's rotation, animation frame, etc.
class SpriteShader {
	friend DrawList;


public:
	class Item {
	public:
		uint32_t texture = 0;
		uint32_t swizzle = 0;
		float frame = 0.f;
		float frameCount = 1.f;
		float position[2] = {0.f, 0.f};
		float transform[4] = {0.f, 0.f, 0.f, 0.f};
		float blur[2] = {0.f, 0.f};
		float clip = 1.f;
		float alpha = 1.f;
	};

private:
	class ShaderState {
	public:
		bool useShaderSwizzle;
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

		static const vector<vector<GLint>> SWIZZLE;
	};
	template <typename T>
	class ShaderImpl {
		friend SpriteShader;


	private:
		static ShaderState state;


	public:
		// Draw a sprite.
		static void Draw(const Sprite *sprite, const Point &position, float zoom = 1.f, int swizzle = 0,
							float frame = 0.f);
		static Item Prepare(const Sprite *sprite, const Point &position, float zoom = 1.f, int swizzle = 0,
							float frame = 0.f);

		static void Bind();
		static void Add(const Item &item, bool withBlur = false);
		static void Unbind();
		static void Init(bool useShaderSwizzle);
	};


public:
	typedef typename SpriteShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename SpriteShader::ShaderImpl<ScaledScreenSpace> UISpace;
	// Initialize the shaders.
	static void Init(bool useShaderSwizzle);
};



template <typename T>
SpriteShader::ShaderState SpriteShader::ShaderImpl<T>::state;



template <typename T>
// Initialize the shaders.
void SpriteShader::ShaderImpl<T>::Init(bool useShaderSwizzle)
{
	state.useShaderSwizzle = useShaderSwizzle;

	static const char *vertexCode =
		"// vertex sprite shader\n"
		"precision mediump float;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 position;\n"
		"uniform mat2 transform;\n"
		"uniform vec2 blur;\n"
		"uniform float clip;\n"

		"in vec2 vert;\n"
		"out vec2 fragTexCoord;\n"

		"void main() {\n"
		"  vec2 blurOff = 2.f * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));\n"
		"  gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);\n"
		"  vec2 texCoord = vert + vec2(.5, .5);\n"
		"  fragTexCoord = vec2(texCoord.x, min(clip, texCoord.y)) + blurOff;\n"
		"}\n";

	ostringstream fragmentCodeStream;
	fragmentCodeStream << "// fragment sprite shader\n"
						"precision mediump float;\n"
#ifdef ES_GLES
						"precision mediump sampler2DArray;\n"
#endif
						"uniform sampler2DArray tex;\n"
						"uniform float frame;\n"
						"uniform float frameCount;\n"
						"uniform vec2 blur;\n";
	if(useShaderSwizzle)
		fragmentCodeStream << "uniform int swizzler;\n";
	fragmentCodeStream << "uniform float alpha;\n"
						"const int range = 5;\n"

						"in vec2 fragTexCoord;\n"

						"out vec4 finalColor;\n"

						"void main() {\n"
						"  float first = floor(frame);\n"
						"  float second = mod(ceil(frame), frameCount);\n"
						"  float fade = frame - first;\n"
						"  vec4 color;\n"
						"  if(blur.x == 0.f && blur.y == 0.f)\n"
						"  {\n"
						"    if(fade != 0.f)\n"
						"      color = mix(\n"
						"        texture(tex, vec3(fragTexCoord, first)),\n"
						"        texture(tex, vec3(fragTexCoord, second)), fade);\n"
						"    else\n"
						"      color = texture(tex, vec3(fragTexCoord, first));\n"
						"  }\n"
						"  else\n"
						"  {\n"
						"    color = vec4(0., 0., 0., 0.);\n"
						"    const float divisor = float(range * (range + 2) + 1);\n"
						"    for(int i = -range; i <= range; ++i)\n"
						"    {\n"
						"      float scale = float(range + 1 - abs(i)) / divisor;\n"
						"      vec2 coord = fragTexCoord + (blur * float(i)) / float(range);\n"
						"      if(fade != 0.f)\n"
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
		fragmentCodeStream << "  switch (swizzler) {\n"
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
							"      color = color.rbba;\n"
							"      break;\n"
							"    case 8:\n"
							"      color = color.rgga;\n"
							"      break;\n"
							"    case 9:\n"
							"      color = color.bbba;\n"
							"      break;\n"
							"    case 10:\n"
							"      color = color.ggga;\n"
							"      break;\n"
							"    case 11:\n"
							"      color = color.rrra;\n"
							"      break;\n"
							"    case 12:\n"
							"      color = color.bbga;\n"
							"      break;\n"
							"    case 13:\n"
							"      color = color.bbra;\n"
							"      break;\n"
							"    case 14:\n"
							"      color = color.ggra;\n"
							"      break;\n"
							"    case 15:\n"
							"      color = color.bgga;\n"
							"      break;\n"
							"    case 16:\n"
							"      color = color.brra;\n"
							"      break;\n"
							"    case 17:\n"
							"      color = color.grra;\n"
							"      break;\n"
							"    case 18:\n"
							"      color = color.bgba;\n"
							"      break;\n"
							"    case 19:\n"
							"      color = color.brba;\n"
							"      break;\n"
							"    case 20:\n"
							"      color = color.grga;\n"
							"      break;\n"
							"    case 21:\n"
							"      color = color.ggba;\n"
							"      break;\n"
							"    case 22:\n"
							"      color = color.rrba;\n"
							"      break;\n"
							"    case 23:\n"
							"      color = color.rrga;\n"
							"      break;\n"
							"    case 24:\n"
							"      color = color.gbga;\n"
							"      break;\n"
							"    case 25:\n"
							"      color = color.rbra;\n"
							"      break;\n"
							"    case 26:\n"
							"      color = color.rgra;\n"
							"      break;\n"
							"    case 27:\n"
							"      color = vec4(color.b, 0.f, 0.f, color.a);\n"
							"      break;\n"
							"    case 28:\n"
							"      color = vec4(0.f, 0.f, 0.f, color.a);\n"
							"      break;\n"
							"  }\n";
	}
	fragmentCodeStream << "  finalColor = color * alpha;\n"
						"}\n";

	static const string fragmentCodeString = fragmentCodeStream.str();
	static const char *fragmentCode = fragmentCodeString.c_str();

	state.shader = Shader(vertexCode, fragmentCode);
	state.scaleI = state.shader.Uniform("scale");
	state.frameI = state.shader.Uniform("frame");
	state.frameCountI = state.shader.Uniform("frameCount");
	state.positionI = state.shader.Uniform("position");
	state.transformI = state.shader.Uniform("transform");
	state.blurI = state.shader.Uniform("blur");
	state.clipI = state.shader.Uniform("clip");
	state.alphaI = state.shader.Uniform("alpha");
	if(state.useShaderSwizzle)
		state.swizzlerI = state.shader.Uniform("swizzler");

	glUseProgram(state.shader.Object());
	glUniform1i(state.shader.Uniform("tex"), 0);
	glUseProgram(0);

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f, .5f,
		.5f, -.5f,
		.5f, .5f};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	glEnableVertexAttribArray(state.shader.Attrib("vert"));
	glVertexAttribPointer(state.shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



// Initialize the shaders.
template <typename T>
void SpriteShader::ShaderImpl<T>::Draw(const Sprite *sprite, const Point &position, float zoom, int swizzle,
									float frame)
{
	if(!sprite)
		return;

	Bind();
	Add(Prepare(sprite, position, zoom, swizzle, frame));
	Unbind();
}



template <typename T>
SpriteShader::Item SpriteShader::ShaderImpl<T>::Prepare(const Sprite *sprite, const Point &position,
														float zoom, int swizzle, float frame)
{
	if(!sprite)
		return {};

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

	return item;
}



template <typename T>
void SpriteShader::ShaderImpl<T>::Bind()
{
	ScreenSpacePtr screenSpace = ScreenSpace::Variant<T>::instance();
	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat scale[2] = {2.f / screenSpace->Width(), -2.f / screenSpace->Height()};
	glUniform2fv(state.scaleI, 1, scale);
}

template <typename T>
void SpriteShader::ShaderImpl<T>::Add(const Item &item, bool withBlur)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, item.texture);

	glUniform1f(state.frameI, item.frame);
	glUniform1f(state.frameCountI, item.frameCount);
	glUniform2fv(state.positionI, 1, item.position);
	glUniformMatrix2fv(state.transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = {0.f, 0.f};
	glUniform2fv(state.blurI, 1, withBlur ? item.blur : UNBLURRED);
	glUniform1f(state.clipI, item.clip);
	glUniform1f(state.alphaI, item.alpha);

	// Bounds check for the swizzle value:
	int swizzle = (static_cast<size_t>(item.swizzle) >= SpriteShader::ShaderState::SWIZZLE.size() ? 0 : item.swizzle);
	// Set the color swizzle.
	if(state.useShaderSwizzle)
		glUniform1i(state.swizzlerI, swizzle);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SpriteShader::ShaderState::SWIZZLE[swizzle].data());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



template <typename T>
void SpriteShader::ShaderImpl<T>::Unbind()
{
	// Reset the swizzle.
	if(state.useShaderSwizzle)
		glUniform1i(state.swizzlerI, 0);
	else
		glTexParameteriv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_RGBA, SpriteShader::ShaderState::SWIZZLE[0].data());

	glBindVertexArray(0);
	glUseProgram(0);
}



#endif
