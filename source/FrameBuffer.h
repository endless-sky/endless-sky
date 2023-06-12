#include "opengl.h"

#include <string>



// All functions for handling framebuffers can be found here.
// The steps to create and use a buffer:
//  1. int myBuffer = FrameBuffer::CreateFrameBuffer();
//  2. int mytexture = FrameBuffer::CreateTextureAttachment(width, height);
//   - creates a texture that can be given to the shader
//   - I recommend using sizes smaller than the window size for post processing
//  3. FrameBuffer::BindFrameBuffer(myBuffer, width, height);
//   - From now on everything will be drawn to the texture, no changes in the drawing process necessary
//  4. execute as many glDrawArrays or glDrawInstanced as you need
//  5. UnbindCurrentFrameBuffer();
//   - you are now back to drawing on the screen and have a texture that contains everything you just drew
class FrameBuffer
{
public:
	struct FrameBufferHandle
	{
		FrameBufferHandle(int width, int height);
		void Bind();

		int buffer;
		int texture;
		int width;
		int height;
	};
public:
	// Creates a framebuffer for color.
	static int CreateFrameBuffer();
	// Create a texture for a color framebuffer.
	static int CreateTextureAttachment(int width, int height);
	// Bind a framebuffer (haha unneccessary comment)
	static void BindFrameBuffer(int buffer, int width, int height);
	// Unbinds any active framebuffer, making the screen drawspace again.
	// It is recommend to call GameWindow::AdjustViewport() after unbinding a buffer.
	static void UnbindCurrentFrameBuffer();

	static void StoreBuffer(std::string id, FrameBufferHandle texture);
	static FrameBufferHandle GetBuffer(std::string id);
};