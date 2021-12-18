/* opengl.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "opengl.h"

#ifdef _WIN32
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif



bool OpenGL::HasAdaptiveVSyncSupport()
{
#ifdef __APPLE__
	// macOS doesn't support Adaptive VSync for OpenGL.
	return false;
#elif defined(_WIN32)
	return wglewIsSupported("WGL_EXT_swap_control_tear");
#else
	return glxewIsSupported("GLX_EXT_swap_control_tear");
#endif
}
