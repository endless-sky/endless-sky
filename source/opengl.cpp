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

#if defined(ES_GLES) || defined(_WIN32)
namespace {
	bool HasOpenGLExtension(const char *name)
	{
#ifndef __APPLE__
		auto extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		return strstr(extensions, name);
#else
		bool value = false;
		GLint extensionCount = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		for(GLint i = 0; i < extensionCount && !value; ++i)
		{
			auto extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
			value = (extension && strstr(extension, name));
		}
		return value;
#endif
	}
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
	return WGL_EXT_swap_control_tear || HasOpenGLExtension("_swap_control_tear");
#else
	return GLX_EXT_swap_control_tear;
#endif
}
