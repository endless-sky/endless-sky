/* opengl.h
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

#pragma once

// Include whichever header is used for OpenGL on this operating system.
#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#ifdef ES_GLES
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif
#endif

#if defined(__APPLE__) || defined(ES_GLES)
#define glBindFramebufferEXT glBindFramebuffer
#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glGenFramebuffersEXT glGenFramebuffers
#endif



// A helper class for various OpenGL platform specific calls.
class OpenGL {
public:
	enum class FeatureSupport {
		NONE = 0,
		EXT,
		CORE
	};


public:
#ifndef ES_GLES
	static void DisableOpenGL3();
#endif

	static bool HasAdaptiveVSyncSupport();
	static bool HasVaoSupport();
	static bool HasTexture2DArraySupport();
	static bool HasClearBufferSupport();
	static FeatureSupport GetFboSupport();
};
