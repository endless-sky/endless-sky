/* FrameBuffer.cpp
Copyright (c) 2023 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FrameBuffer.h"

#include <map>



namespace {
	std::map<std::string, int> textureStorage;
}



int FrameBuffer::CreateFrameBuffer()
{
	GLuint frameBuffer;
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	return frameBuffer;
}



int FrameBuffer::CreateTextureAttachment(int width, int height)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, // target, mipmap level, internal format,
		width, height, 1,          // width, height, depth,
		0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);   // border, input format, data type, data.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
	return texture;
}



void FrameBuffer::BindFrameBuffer(int buffer, int width, int height)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	glViewport(0, 0, width, height);
}



void FrameBuffer::UnbindCurrentFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void FrameBuffer::DestroyBuffer(GLuint buffer, GLuint texture)
{
	glDeleteFramebuffers(1, &buffer);
	glDeleteTextures(1, &texture);
}



void FrameBuffer::StoreTexture(std::string id, int texture)
{
	textureStorage[id] = texture;
}



int FrameBuffer::GetTexture(std::string id)
{
	return textureStorage[id];
}