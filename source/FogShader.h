/* FogShader.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FOG_SHADER_H_
#define FOG_SHADER_H_

#include "AbsoluteScreenSpace.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "ScaledScreenSpace.h"
#include "ScreenSpace.h"
#include "Shader.h"
#include "System.h"

#include "opengl.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace std;

// Shader for drawing a "fog of war" overlay on the map.
class FogShader {
private:
	class ShaderState {
	friend FogShader;
	public:
		// Scale of the mask image:
		static const int GRID = 16;
		// Distance represented by one orthogonal or diagonal step:
		static const int ORTH = 5;
		static const int DIAG = 7;
		// Limit distances to the size of an unsigned char.
		static const int LIMIT = 255;
		// Pad beyond the screen enough to include any system that might "cast light"
		// on the on-screen view.
		static const int PAD = LIMIT / ORTH;

		// OpenGL objects:
		Shader shader;
		GLuint cornerI;
		GLuint dimensionsI;
		GLuint vao;
		GLuint vbo;
		GLuint texture = 0;

		// Keep track of the previous frame's view so that if it is unchanged we can
		// skip regenerating the mask.
		double previousZoom = 0.;
		double previousLeft = 0.;
		double previousTop = 0.;
		int previousColumns = 0;
		int previousRows = 0;
		Point previousCenter;
	};

	template<typename T>
	class ShaderImpl {
	private:
		static ShaderState state;
	public:
		static void Init();
		static void Redraw();
		static void Draw(const Point &center, double zoom, const PlayerInfo &player);
	};
public:
	static void Init();
	typedef typename FogShader::ShaderImpl<AbsoluteScreenSpace> ViewSpace;
	typedef typename FogShader::ShaderImpl<ScaledScreenSpace> UISpace;
};


template<typename T>
FogShader::ShaderState FogShader::ShaderImpl<T>::state;

template<typename T>
void FogShader::ShaderImpl<T>::Init()
{
	static const char *vertexCode =
		"// vertex fog shader\n"
		"uniform vec2 corner;\n"
		"uniform vec2 dimensions;\n"

		"in vec2 vert;\n"
		"out vec2 fragTexCoord;\n"

		"void main() {\n"
		"  gl_Position = vec4(corner + vert * dimensions, 0, 1);\n"
		"  fragTexCoord = vert;\n"
		"}\n";

	static const char *fragmentCode =
		"// fragment fog shader\n"
#ifdef ES_GLES
		"precision mediump sampler2D;\n"
#endif
		"precision mediump float;\n"
		"uniform sampler2D tex;\n"

		"in vec2 fragTexCoord;\n"
		"out vec4 finalColor;\n"

		"void main() {\n"
		"  finalColor = vec4(0, 0, 0, texture(tex, fragTexCoord).r);\n"
		"}\n";

	// Compile the shader and store indices to its variables.
	state.shader = Shader(vertexCode, fragmentCode);
	state.cornerI = state.shader.Uniform("corner");
	state.dimensionsI = state.shader.Uniform("dimensions");

	glUseProgram(state.shader.Object());
	glUniform1i(state.shader.Uniform("tex"), 0);
	glUseProgram(0);

	// Generate the vertex data for drawing sprites.
	glGenVertexArrays(1, &state.vao);
	glBindVertexArray(state.vao);

	glGenBuffers(1, &state.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.vbo);

	// Corners of a rectangle to draw.
	GLfloat vertexData[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	GLuint vertI = state.shader.Attrib("vert");
	glEnableVertexAttribArray(vertI);
	glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

	// Unbind the VBO and VAO.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


template<typename T>
void FogShader::ShaderImpl<T>::Redraw()
{
	state.previousZoom = 0.;
}

template<typename T>
void FogShader::ShaderImpl<T>::Draw(const Point &center, double zoom, const PlayerInfo &player)
{
	static const int
		GRID = FogShader::ShaderState::GRID,
		PAD = FogShader::ShaderState::PAD,
		ORTH = FogShader::ShaderState::ORTH,
		DIAG = FogShader::ShaderState::DIAG,
		LIMIT = FogShader::ShaderState::LIMIT;
	static std::shared_ptr<ScreenSpace> screenSpace = ScreenSpace::Variant<T>::instance();

	// Generate a scaled-down mask image that represents the entire screen plus
	// enough pixels beyond the screen to include any systems that may be off
	// screen but close enough to "illuminate" part of the on-screen map.
	double left = screenSpace->Left() - GRID * PAD * zoom + fmod(center.X(), GRID) * zoom;
	double top = screenSpace->Top() - GRID * PAD * zoom + fmod(center.Y(), GRID) * zoom;
	int columns = ceil(screenSpace->Width() / (GRID * zoom)) + 1 + 2 * PAD;
	int rows = ceil(screenSpace->Height() / (GRID * zoom)) + 1 + 2 * PAD;
	// Round up to a multiple of 4 so the rows will be 32-bit aligned.
	columns = (columns + 3) & ~3;

	// To avoid extra work, don't regenerate the mask buffer if the view has not
	// moved. This might cause an inaccurate mask if you explore more systems,
	// come back to the original, and view the map again without viewing it in
	// between. But, that's an unlikely situation.
	bool shouldRegenerate = (
		zoom != state.previousZoom || center.X() != state.previousCenter.X() ||
		center.Y() != state.previousCenter.Y() || left != state.previousLeft ||
		top != state.previousTop || columns != state.previousColumns ||
		rows != state.previousRows);
	if(shouldRegenerate)
	{
		bool sizeChanged = (!state.texture || columns != state.previousColumns || rows != state.previousRows);

		// Remember the current viewport attributes.
		state.previousZoom = zoom;
		state.previousCenter = center;
		state.previousLeft = left;
		state.previousTop = top;
		state.previousColumns = columns;
		state.previousRows = rows;

		// This buffer will hold the mask image.
		auto buffer = vector<unsigned char>(static_cast<size_t>(rows) * columns, LIMIT);

		// For each system the player knows about, its "distance" pixel in the
		// buffer should be set to 0.
		for(const auto &it : GameData::Systems())
		{
			const System &system = it.second;
			if(!system.IsValid() || !player.HasVisited(system))
				continue;
			Point pos = zoom * (system.Position() + center);

			int x = round((pos.X() - left) / (GRID * zoom));
			int y = round((pos.Y() - top) / (GRID * zoom));
			if(x >= 0 && y >= 0 && x < columns && y < rows)
				buffer[x + y * columns] = 0;
		}

		// Distance transformation: make two passes through the buffer. In the first
		// pass, propagate down and to the right. In the second, propagate in the
		// opposite direction. Once these two passes are done, each value is equal
		for(int y = 1; y < rows; ++y)
			for(int x = 1; x < columns; ++x)
				buffer[x + y * columns] = min<int>(buffer[x + y * columns], min(
					ORTH + min(buffer[(x - 1) + y * columns], buffer[x + (y - 1) * columns]),
					DIAG + min(buffer[(x - 1) + (y - 1) * columns], buffer[(x + 1) + (y - 1) * columns])));
		for(int y = rows - 2; y >= 0; --y)
			for(int x = columns - 2; x >= 0; --x)
				buffer[x + y * columns] = min<int>(buffer[x + y * columns], min(
					ORTH + min(buffer[(x + 1) + y * columns], buffer[x + (y + 1) * columns]),
					DIAG + min(buffer[(x - 1) + (y + 1) * columns], buffer[(x + 1) + (y + 1) * columns])));

		// Stretch the distance values so there is no shading up to about 200 pixels
		// away, then it transitions somewhat quickly.
		for(unsigned char &value : buffer)
			value = max(0, min(LIMIT, (value - 60) * 4));
		const void *data = &buffer.front();

		// Set up the OpenGL texture if it doesn't exist yet.
		if(sizeChanged)
		{
			// If the texture size changed, it must be reallocated.
			if(state.texture)
				glDeleteTextures(1, &state.texture);

			glGenTextures(1, &state.texture);
			glBindTexture(GL_TEXTURE_2D, state.texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Upload the new "image."
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, columns, rows, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, state.texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, columns, rows, GL_RED, GL_UNSIGNED_BYTE, data);
		}
	}
	else
		glBindTexture(GL_TEXTURE_2D, state.texture);

	// Set up to draw the image.
	glUseProgram(state.shader.Object());
	glBindVertexArray(state.vao);

	GLfloat corner[2] = {
		static_cast<float>(left - .5 * GRID * zoom) / (.5f * screenSpace->Width()),
		static_cast<float>(top - .5 * GRID * zoom) / (-.5f * screenSpace->Height())};
	glUniform2fv(state.cornerI, 1, corner);
	GLfloat dimensions[2] = {
		GRID * static_cast<float>(zoom) * (columns + 1.f) / (.5f * screenSpace->Width()),
		GRID * static_cast<float>(zoom) * (rows + 1.f) / (-.5f * screenSpace->Height())};
	glUniform2fv(state.dimensionsI, 1, dimensions);

	// Call the shader program to draw the image.
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Clean up.
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


#endif
