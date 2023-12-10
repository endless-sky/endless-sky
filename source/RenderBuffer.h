/* RenderBuffer.h
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

#ifndef RENDERBUFFER_H
#define RENDERBUFFER_H

#include "Point.h"
#include "Rectangle.h"


// TODO: put this in some header somewhere?
// TODO: C++17 has [[nodiscard]] attribute
#ifdef __GNUC__
#define WARN_UNUSED  __attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif


// Class that can redirect all drawing commands to an internal texture.
// This buffer uses coordinates from 0, 0 in the top left, to width, height
// in the bottom right.
class RenderBuffer
{
public:
	// Create a texture of the given size that can be used as a render target
	RenderBuffer(const Point & dimensions);
	virtual ~RenderBuffer();

	// Initialize the shaders used internally
	static void Init();

	// Use RAII to control render target
	class Activation
	{
	public:
		virtual ~Activation() { Deactivate(); }

		// Explicitly deactivate render target;
		void Deactivate() { m_buffer.Deactivate(); }

	protected:
		Activation(RenderBuffer & b) : m_buffer(b) {}
	
	private:
		RenderBuffer & m_buffer;
		friend class RenderBuffer;
	};

	// Turn this buffer on as a render target. The render target is restored if
	// the Activation object goes out of scope.
	Activation SetTarget() WARN_UNUSED;

	// Draw the contents of this buffer at the specified position.
	void Draw(const Point & position) { Draw(position, m_size); };
	// Draw the contents of this buffer at the specified position, clipping the contents
	void Draw(const Point & position, const Point & clipsize, const Point & srcposition = Point());

	double Top() const { return -m_size.Y() / 2; }
	double Bottom() const { return m_size.Y() / 2; }
	double Left() const { return -m_size.X() / 2; }
	double Right() const { return m_size.X() / 2; }
	const Point & Dimensions() const { return m_size; }
	double Height() const { return m_size.Y(); }
	double Width() const { return m_size.X(); }

protected:
	void Deactivate();

	Point m_size;
	unsigned int m_texid = -1;
	unsigned int m_framebuffer = -1;
	unsigned int m_last_framebuffer = 0;
	int m_last_viewport[4] = {};
	int m_old_width = 0;
	int m_old_height = 0;
};


#endif


