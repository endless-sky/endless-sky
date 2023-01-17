/* BatchShader.h
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BATCH_SHADER_H_
#define BATCH_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"
#include "Sprite.h"

#include <memory>
#include <vector>


class BatchDrawList;

using namespace std;

// Class for drawing sprites in a batch. The input to each draw command is a
// sprite, whether it should be drawn high DPI, and the vertex data.
class BatchShader {
friend BatchDrawList;
private:
	class ShaderState {
	public:
		Shader shader;
		// Uniforms:
		GLint scaleI;
		GLint frameCountI;
		// Vertex data:
		GLint vertI;
		GLint texCoordI;

		GLuint vao;
		GLuint vbo;
	};

	template<typename T>
	class ShaderImpl {
	private:
		static ShaderState state;
	public:
		static void Init();

		static void Bind();
		static void Add(const Sprite *sprite, bool isHighDPI, const std::vector<float> &data);
		static void Unbind();
	};
public:
	// Initialize the shaders.
	static void Init();

	typedef typename BatchShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename BatchShader::ShaderImpl<ScaledScreenSpace> UISpace;
};

template<typename T>
BatchShader::ShaderState BatchShader::ShaderImpl<T>::state;

// Initialize the shaders.
template<typename T>
void BatchShader::ShaderImpl<T>::Init()
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
		"precision mediump float;\n"
#ifdef ES_GLES
		"precision mediump sampler2DArray;\n"
#endif
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
	state.shader = Shader(vertexCode, fragmentCode);
	// Get the indices of the uniforms and attributes.
	state.scaleI = state.shader.Uniform("scale");
	state.frameCountI = state.shader.Uniform("frameCount");
	state.vertI = state.shader.Attrib("vert");
	state.texCoordI = state.shader.Attrib("texCoord");

	// Make sure we're using texture 0.
	glUseProgram(state.shader.Object());
	glUniform1i(state.shader.Uniform("tex"), 0);
	glUseProgram(0);

	// Generate the buffer for uploading the batch vertex data.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	// In this VAO, enable the two vertex arrays and specify their byte offsets.
	constexpr auto stride = 5 * sizeof(float);
	glEnableVertexAttribArray(state.vertI);
	glVertexAttribPointer(state.vertI, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
	// The 3 texture fields (s, t, frame) come after the x,y pixel fields.
	auto textureOffset = reinterpret_cast<const GLvoid *>(2 * sizeof(float));
	glEnableVertexAttribArray(state.texCoordI);
	glVertexAttribPointer(state.texCoordI, 3, GL_FLOAT, GL_FALSE, stride, textureOffset);

	// Unbind the buffer and the VAO, but leave the vertex attrib arrays enabled
	// in the VAO so they will be used when it is bound.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



template<typename T>
void BatchShader::ShaderImpl<T>::Bind()
{
	std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();
	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);
	// Bind the vertex buffer so we can upload data to it.
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	// Set up the screen scale.
	GLfloat scale[2] = {2.f / screenSpace->Width(), -2.f / screenSpace->Height()};
	glUniform2fv(state.scaleI, 1, scale);
}



template<typename T>
void BatchShader::ShaderImpl<T>::Add(const Sprite *sprite, bool isHighDPI, const vector<float> &data)
{
	// Do nothing if there are no sprites to draw.
	if(data.empty())
		return;

	// First, bind the proper texture.
	glBindTexture(GL_TEXTURE_2D_ARRAY, sprite->Texture(isHighDPI));
	// The shader also needs to know how many frames the texture has.
	glUniform1f(state.frameCountI, sprite->Frames());

	// Upload the vertex data.
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), data.data(), GL_STREAM_DRAW);

	// Draw all the vertices.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, data.size() / 5);
}



template<typename T>
void BatchShader::ShaderImpl<T>::Unbind()
{
	// Unbind everything in reverse order.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}



#endif
