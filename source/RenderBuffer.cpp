/* RenderBuffer.cpp
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "RenderBuffer.h"

#include "GameData.h"
#include "GameWindow.h"
#include "Logger.h"
#include "Screen.h"
#include "shader/Shader.h"

#include "opengl.h"

using namespace std;

namespace {
	const Shader *shader;
	GLint sizeI = -1;
	GLint positionI = -1;
	GLint scaleI = -1;
	GLint srcpositionI = -1;
	GLint srcscaleI = -1;
	GLint fadeI = -1;

	GLint vertI;

	GLuint vao = -1;
	GLuint vbo = -1;

	void EnableAttribArrays()
	{
		glEnableVertexAttribArray(vertI);
		glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	}
}



// Initialize the shaders.
void RenderBuffer::Init()
{
	shader = GameData::Shaders().Get("renderBuffer");
	if(!shader->Object())
		throw runtime_error("Could not find render buffer shader!");
	sizeI = shader->Uniform("size");
	positionI = shader->Uniform("position");
	scaleI = shader->Uniform("scale");
	srcpositionI = shader->Uniform("srcposition");
	srcscaleI = shader->Uniform("srcscale");
	fadeI = shader->Uniform("fade");
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

	// Unbind the VBO and VAO.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
}



RenderBuffer::RenderTargetGuard::~RenderTargetGuard()
{
	Deactivate();
}



void RenderBuffer::RenderTargetGuard::Deactivate()
{
	buffer.Deactivate();
	screenGuard.Deactivate();
}



RenderBuffer::RenderTargetGuard::RenderTargetGuard(RenderBuffer &b, int screenWidth, int screenHeight)
	: buffer(b), screenGuard(screenWidth, screenHeight)
{
}



// Create a texture of the given size that can be used as a render target.
RenderBuffer::RenderBuffer(const Point &dimensions)
	: size(dimensions)
{
	// Generate a framebuffer.
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// Generate the texture.
	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);

	// Use nearest pixel and no wrapping.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	multiplier = Point(GameWindow::DrawWidth() / Screen::RawWidth(), GameWindow::DrawHeight() / Screen::RawHeight());

	// Attach a blank image to the texture.
	const Point scaledSize = size * multiplier * Screen::Zoom() / 100.0;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaledSize.X(), scaledSize.Y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Attach the texture to the frame buffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, draw_buffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		Logger::Log("Failed to initialize framebuffer for RenderBuffer.", Logger::Level::WARNING);


	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Default to the current viewport size at the time of construction.
	glGetIntegerv(GL_VIEWPORT, lastViewport);
}



// Destructor. Frees the texture and renderbuffers.
RenderBuffer::~RenderBuffer()
{
	glDeleteTextures(1, &texid);
	glDeleteFramebuffers(1, &framebuffer);
}



// Turn this buffer on as a render target. The render target is restored if
// the Activation object goes out of scope.
RenderBuffer::RenderTargetGuard RenderBuffer::SetTarget()
{
	// NOTE: These glGets can cause an unwanted state synchronization that can
	//       cause performance problems. The only real reason we might want this
	//       is if we are nesting render buffers. If only one framebuffer is
	//       enabled at a time, then we can just reset the buffer to 0 when we
	//       are done.
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<int*>(&lastFramebuffer));
	glGetIntegerv(GL_VIEWPORT, lastViewport);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	const Point scaledSize = size * multiplier * Screen::Zoom() / 100.0;
	glViewport(0, 0, scaledSize.X(), scaledSize.Y());

	static const float CLEAR[] = {0, 0, 0, 0};
	if(OpenGL::HasClearBufferSupport())
		glClearBufferfv(GL_COLOR, 0, CLEAR);
	else
	{
		GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffers);
		glClearColor(CLEAR[0], CLEAR[1], CLEAR[2], CLEAR[3]);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	return RenderTargetGuard(*this, size.X(), size.Y());
}



// Reset the render target and viewport to the original settings.
void RenderBuffer::Deactivate()
{
	// Restore the old settings.
	glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, lastFramebuffer);
}



void RenderBuffer::Draw(const Point &position)
{
	Draw(position, size);
}



// Draw the contents of this buffer at the specified position.
void RenderBuffer::Draw(const Point &position, const Point &clipsize, const Point &srcposition)
{
	glUseProgram(shader->Object());
	if(OpenGL::HasVaoSupport())
		glBindVertexArray(vao);
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		EnableAttribArrays();
	}

	glBindTexture(GL_TEXTURE_2D, texid);

	glUniform2f(sizeI, clipsize.X(), clipsize.Y());
	glUniform2f(positionI, position.X(), position.Y());
	glUniform2f(scaleI, 2.f / Screen::Width(), -2.f / Screen::Height());

	glUniform2f(srcpositionI, srcposition.X(), srcposition.Y());
	glUniform2f(srcscaleI, 1.f / size.X(), 1.f / size.Y());

	glUniform4f(fadeI,
		fadePadding[0] / clipsize.Y(),
		fadePadding[1] / clipsize.Y(),
		fadePadding[2] / clipsize.X(),
		fadePadding[3] / clipsize.X()
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if(OpenGL::HasVaoSupport())
		glBindVertexArray(0);
	else
	{
		glDisableVertexAttribArray(vertI);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glUseProgram(0);
}



double RenderBuffer::Top() const
{
	return -size.Y() / 2;
}



double RenderBuffer::Bottom() const
{
	return size.Y() / 2;
}



double RenderBuffer::Left() const
{
	return -size.X() / 2;
}



double RenderBuffer::Right() const
{
	return size.X() / 2;
}



const Point &RenderBuffer::Dimensions() const
{
	return size;
}



double RenderBuffer::Height() const
{
	return size.Y();
}



double RenderBuffer::Width() const
{
	return size.X();
}



void RenderBuffer::SetFadePadding(float top, float bottom, float left, float right)
{
	fadePadding[0] = top;
	fadePadding[1] = bottom;
	fadePadding[2] = left;
	fadePadding[3] = right;
}
