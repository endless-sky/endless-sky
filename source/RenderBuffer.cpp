/* RenderBuffer.cpp
Copyright (c) 2023 by Rian Shelley

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

#include "Logger.h"
#include "Screen.h"
#include "Shader.h"

#include "opengl.h"
#include <SDL2/SDL_log.h>

namespace {
	Shader shader;
	GLint sizeI = -1;
	GLint positionI = -1;
	GLint scaleI = -1;
	GLint srcpositionI = -1;
	GLint srcscaleI = -1;

	GLint texI = -1;

	GLuint vao = -1;
	GLuint vbo = -1;
}

// Initialize the shaders.
void RenderBuffer::Init()
{
	static const char *vertexCode =
		"// vertex blit shader\n"
		"precision mediump float;\n"
		"uniform vec2 size;\n"
		"uniform vec2 position;\n"
		"uniform vec2 scale;\n"
		
		"uniform vec2 srcposition;\n"
		"uniform vec2 srcscale;\n"
		
		"in vec2 vert;\n"
		"out vec2 tpos;\n"
		
		"void main() \n"
		"{\n"
		"  gl_Position = vec4((position + vert * size) * scale, 0, 1);\n"
		"  vec2 tvert = vert + vec2(.5, .5);\n" // Convert from vertex to texture coordinates
		"  vec2 tsize = size * srcscale;\n"    // Convert from screen to texture coordinates
		"  vec2 tsrc = srcposition * srcscale;\n" // Convert from screen to texture coordinates
		"  tpos = tvert * tsize + tsrc;\n"
		"  tpos.y = 1.0 - tpos.y;\n" // negative is up.
		"}\n";

	static const char *fragmentCode =
		"// fragment blit shader\n"
		"precision mediump float;\n"
		"precision mediump sampler2D;\n"
		"uniform sampler2D tex;\n"
		
		"in vec2 tpos;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  if(tpos.x > 0.0 && tpos.y > 0.0 &&\n"
		"      tpos.x < 1.0 && tpos.y < 1.0 )\n"
		"    finalColor = texture(tex, tpos);\n"
		"  else\n"
		"    discard;\n"
		"}\n";

	shader = Shader(vertexCode, fragmentCode);
	sizeI = shader.Uniform("size");
	positionI = shader.Uniform("position");
	scaleI = shader.Uniform("scale");
	srcpositionI = shader.Uniform("srcposition");
	srcscaleI = shader.Uniform("srcscale");
	texI = shader.Uniform("tex");

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



// Create a texture of the given size that can be used as a render target
RenderBuffer::RenderBuffer(const Point& dimensions):
   m_size(dimensions)
{
	// Generate a framebuffer, and bind it.
	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	// Generate the texture
	glGenTextures(1, &m_texid);
	glBindTexture(GL_TEXTURE_2D, m_texid);

	// Use linear interpolation and no wrapping.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Attach a blank image to the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.X(), m_size.Y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Attach the texture to the frame buffer
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texid, 0);
	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, draw_buffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		Logger::LogError("Failed to initialize framebuffer for RenderBuffer");
	}


	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// default to the current viewport size at the time of construction
	glGetIntegerv(GL_VIEWPORT, m_last_viewport);

	// save off the screen size for the game coordinates (not the same as
	// the window size or the viewport size)
	m_old_width = Screen::Width();
	m_old_height = Screen::Height();
}



// Destructor. Frees the texture and renderbuffers
RenderBuffer::~RenderBuffer()
{
	glDeleteTextures(1, &m_texid);
	glDeleteFramebuffers(1, &m_framebuffer);
}



// Turn this buffer on as a render target. The render target is restored if
// the Activation object goes out of scope.
RenderBuffer::Activation RenderBuffer::SetTarget()
{
	// NOTE: These glGets can cause an unwanted state synchronization that can
	//       cause performance problems. The only real reason we might want this
	//       is if we are nesting render buffers. If only one framebuffer is
	//       enabled at a time, then we can just reset the buffer to 0 when we
	//       are done.
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<int*>(&m_last_framebuffer));
	glGetIntegerv(GL_VIEWPORT, m_last_viewport);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);


	glViewport(0, 0, m_size.X(), m_size.Y());
	m_old_width = Screen::Width();
	m_old_height = Screen::Height();

	Screen::SetDimensionsInternal(m_size.X(), m_size.Y());

	static const float CLEAR[] = {0, 0, 0, 0};
	glClearBufferfv(GL_COLOR, 0, CLEAR);

	return Activation(*this);
}



// Reset the render target and viewport to the original settings
void RenderBuffer::Deactivate()
{
	// Restore the old settings
	Screen::SetDimensionsInternal(m_old_width, m_old_height);
	glViewport(m_last_viewport[0], m_last_viewport[1],
	           m_last_viewport[2], m_last_viewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, m_last_framebuffer);
}



// Draw the contents of this buffer at the specified position.
void RenderBuffer::Draw(const Point& position, const Point& clipsize, const Point& srcposition)
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);

	glUniform1i(texI, 0);
	glBindTexture(GL_TEXTURE_2D, m_texid);

	glUniform2f(sizeI, clipsize.X(), clipsize.Y());
	glUniform2f(positionI, position.X(), position.Y());
	glUniform2f(scaleI, 2.f / Screen::Width(), -2.f / Screen::Height());

	glUniform2f(srcpositionI, srcposition.X(), srcposition.Y());
	glUniform2f(srcscaleI, 1.f / m_size.X(), 1.f / m_size.Y());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
	glUseProgram(0);
}
