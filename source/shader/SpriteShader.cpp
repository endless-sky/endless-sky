/* SpriteShader.cpp
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

#include "SpriteShader.h"

#include "../GameData.h"
#include "../Screen.h"
#include "Shader.h"
#include "../image/Sprite.h"
#include "../Swizzle.h"

using namespace std;

namespace {
	const Shader *shader;
	GLint scaleI;
	GLint texI;
	GLint swizzleMaskI;
	GLint useSwizzleMaskI;
	GLint frameI;
	GLint frameCountI;
	GLint positionI;
	GLint transformI;
	GLint blurI;
	GLint clipI;
	GLint alphaI;
	GLint swizzleMatrixI;
	GLint useSwizzleI;

	GLint vertI;

	GLuint vao;
	GLuint vbo;

	void EnableAttribArrays()
	{
		glEnableVertexAttribArray(vertI);
		glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	}
}

// Initialize the shaders.
void SpriteShader::Init()
{
	shader = GameData::Shaders().Get("sprite");
	if(!shader->Object())
		throw runtime_error("Could not find sprite shader!");
	scaleI = shader->Uniform("scale");
	texI = shader->Uniform("tex");
	frameI = shader->Uniform("frame");
	frameCountI = shader->Uniform("frameCount");
	positionI = shader->Uniform("position");
	transformI = shader->Uniform("transform");
	blurI = shader->Uniform("blur");
	clipI = shader->Uniform("clip");
	alphaI = shader->Uniform("alpha");
	swizzleMatrixI = shader->Uniform("swizzleMatrix");
	swizzleMaskI = shader->Uniform("swizzleMask");
	useSwizzleMaskI = shader->Uniform("useSwizzleMask");
	useSwizzleI = shader->Uniform("useSwizzle");
	vertI = shader->Attrib("vert");

	// Generate the vertex data for drawing sprites.
	if(OpenGL::HasVaoSupport())
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	if(OpenGL::HasVaoSupport())
		EnableAttribArrays();

	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



void SpriteShader::Draw(const Sprite *sprite, const Point &position,
	float zoom, const Swizzle *swizzle, float frame, const Point &unit)
{
	if(!sprite)
		return;

	Bind();
	Add(Prepare(sprite, position, zoom, swizzle, frame, unit));
	Unbind();
}



SpriteShader::Item SpriteShader::Prepare(const Sprite *sprite, const Point &position,
	float zoom, const Swizzle *swizzle, float frame, const Point &unit)
{
	if(!sprite)
		return {};

	Item item;
	item.texture = sprite->Texture();
	item.swizzleMask = sprite->SwizzleMask();
	item.frame = frame;
	item.frameCount = sprite->Frames();
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());
	// Rotation and scale.
	Point scaledUnit = unit * zoom;
	Point uw = scaledUnit * sprite->Width();
	Point uh = scaledUnit * sprite->Height();
	item.transform[0] = static_cast<float>(-uw.Y());
	item.transform[1] = static_cast<float>(uw.X());
	item.transform[2] = static_cast<float>(-uh.X());
	item.transform[3] = static_cast<float>(-uh.Y());
	// Swizzle.
	item.swizzle = swizzle;

	return item;
}



void SpriteShader::Bind()
{
	glUseProgram(shader->Object());
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(vao);
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		EnableAttribArrays();
	}

	GLfloat scale[2] = {2.f / Screen::Width(), -2.f / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
}



void SpriteShader::Add(const Item &item, bool withBlur)
{
	if(item.swizzle)
	{
		glUniform1i(swizzleMaskI, 1);
		// Don't mask full color swizzles that always apply to the whole ship sprite.
		glUniform1i(useSwizzleMaskI, item.swizzle->OverrideMask() ? 0 : item.swizzleMask);

		// Set the color swizzle.
		glUniformMatrix4fv(swizzleMatrixI, 1, GL_FALSE, item.swizzle->MatrixPtr());
		glUniform1i(useSwizzleI, !item.swizzle->IsIdentity());
	}
	else
		glUniform1i(useSwizzleI, 0);

	glUniform1i(texI, 0);
	int type = OpenGL::HasTexture2DArraySupport() ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_3D;
	glBindTexture(type, item.texture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(type, item.swizzleMask);
	glActiveTexture(GL_TEXTURE0);

	glUniform1f(frameI, item.frame);
	glUniform1f(frameCountI, item.frameCount);
	glUniform2fv(positionI, 1, item.position);
	glUniformMatrix2fv(transformI, 1, false, item.transform);
	// Special case: check if the blur should be applied or not.
	static const float UNBLURRED[2] = {0.f, 0.f};
	glUniform2fv(blurI, 1, withBlur ? item.blur : UNBLURRED);
	glUniform1f(clipI, item.clip);
	glUniform1f(alphaI, item.alpha);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}



void SpriteShader::Unbind()
{
	// Reset the swizzle.
	glUniform1i(useSwizzleI, 0);

	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
	else
	{
		glDisableVertexAttribArray(vertI);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glUseProgram(0);
}
