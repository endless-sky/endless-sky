#include "FrameBuffer.h"

#include <map>



namespace {
	std::map<std::string, FrameBufferHandle> bufferStorage;
};



void FrameBuffer::FrameBufferHandle::FrameBufferHandle(int width, int height)
{
	width, height = width, height;
	buffer = CreateFrameBuffer();
	texture = CreateTextureAttachment(width, height);
	UnbindCurrentFrameBuffer();
}



void FrameBuffer::FrameBufferHandle::Bind()
{
	BindFrameBuffer(width, height);
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
	// target, mipmap level, internal format, width, height, depth, border, input format, data type, data.
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, width, height, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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



void FrameBuffer::StoreBuffer(std::string id, FrameBuffer::FrameBufferHandle buffer)
{
	bufferStorage[id] = buffer;
}



FrameBuffer::FrameBufferHandle FrameBuffer::GetBuffer(std::string id)
{
	return bufferStorage[id];
}