/* FrameBuffer.h
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

#include <string>

#include "opengl.h"



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
//  6. IMPORTANT, dont skip this step: DestroyBuffer(myBuffer, mytexture)
//   - If you skip this step you will allocate more and more memory over time and crash the system.
//    (or activate failsafes)
class FrameBuffer
{
public:
	// Creates a framebuffer for color.
	static int CreateFrameBuffer();
	// Create a texture for a color framebuffer.
	static int CreateTextureAttachment(int width, int height);
	// Bind a framebuffer (haha unneccessary comment)
	static void BindFrameBuffer(int buffer, int width, int height);
	// Unbinds any active framebuffer, making the screen drawspace again
	static void UnbindCurrentFrameBuffer();
	// Clears up memory of the buffer and the texture, only do after using the texture
	static void DestroyBuffer(GLuint buffer, GLuint texture);

	static void StoreTexture(std::string id, int texture);
	static int GetTexture(std::string id);
};
