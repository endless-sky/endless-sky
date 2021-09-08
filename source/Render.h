/* Render.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef RENDER_H_
#define RENDER_H_

#include <string>

class Sprite;
class StarField;



// Functions that are used for rendering related stuff. These
// are not defined in the static library of the game so that
// it is possible to build the game without rendering support.
namespace Render {
	// Initial load of music and images.
	void Load();
	void LoadShaders(bool useShaderSwizzle);
	void LoadPlugin(const std::string &path, const std::string &name);

	// TODO: make Progress() a simple accessor.
	double Progress();
	// Whether initial game loading is complete (sprites and audio are loaded).
	bool IsLoaded();

	// Begin loading a sprite that was previously deferred. Currently this is
	// done with all landscapes to speed up the program's startup.
	void Preload(const Sprite *sprite);
	void FinishLoading();

	const StarField &Background();
	void SetHaze(const Sprite *sprite, bool allowAnimation);
}



#endif
