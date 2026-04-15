/* SDL.h
Copyright (c) 2026 by tibetiroka

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

/// Provides compatibility mappings between SDL2 and SDL3. Most of the game code uses the old SDL2 names and functions.

#ifdef ES_USE_SDL3

#ifndef SDL_ENABLE_OLD_NAMES
#define SDL_ENABLE_OLD_NAMES
#endif
#include <SDL3/SDL.h>

using mouse_pos_t = float;

// Some missed old names
#ifndef SDL_HINT_VIDEODRIVER
#define SDL_HINT_VIDEODRIVER SDL_HINT_VIDEO_DRIVER
#endif

// These are defined to warn you about small api changes, not relevant for us
#ifdef SDL_WINDOW_FULLSCREEN_DESKTOP
#undef SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
#define SDL_WINDOW_FULLSCREEN_DESKTOP SDL_WINDOW_FULLSCREEN

#ifdef SDL_GL_GetDrawableSize
#undef SDL_GL_GetDrawableSize
#endif
// This one handles screen scaling and other weirdness differently,
// but that was unreliable in SDL2 and had numerous bug reports.
#define SDL_GL_GetDrawableSize(window, width, height) SDL_GetWindowSizeInPixels((window), (width), (height));

#else
#include <SDL2/SDL.h>

using mouse_pos_t = int;
#endif
