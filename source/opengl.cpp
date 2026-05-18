/* opengl.cpp
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

#include "opengl.h"

#if !defined(__APPLE__) && !defined(ES_GLES)
#ifdef _WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif
#endif

#include <cstring>

namespace {
	bool hasOpenGL3Support = true;

#if defined(ES_GLES) || defined(_WIN32)
	bool HasOpenGLExtension(const char *name)
	{
		auto extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		return strstr(extensions, name);
	}
#endif
}



#ifndef ES_GLES
void OpenGL::DisableOpenGL3()
{
	hasOpenGL3Support = false;
}
#endif



bool OpenGL::HasAdaptiveVSyncSupport()
{
#ifdef __APPLE__
	// macOS doesn't support Adaptive VSync for OpenGL.
	return false;
#elif defined(ES_GLES)
	return HasOpenGLExtension("_swap_control_tear");
#elif defined(_WIN32)
	return WGLEW_EXT_swap_control_tear || HasOpenGLExtension("_swap_control_tear");
#else
	return true;
#endif
}



bool OpenGL::HasVaoSupport()
{
	// TODO: Add an extension check if we want to enable VAOs on more devices.
	return hasOpenGL3Support;
}



bool OpenGL::HasTexture2DArraySupport()
{
	// TODO: Add an extension check if we want to enable texture arrays on more devices.
	return hasOpenGL3Support;
}



bool OpenGL::HasClearBufferSupport()
{
	return hasOpenGL3Support;
}
