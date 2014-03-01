/* StarField.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef STAR_FIELD_H_
#define STAR_FIELD_H_

#include "Point.h"
#include "Shader.h"

#include <GL/glew.h>

#include <vector>



// Object to hold a set of "stars" to be drawn as a backdrop.
class StarField {
public:
	void Init(int stars, int width);
	
	void Draw(Point pos, Point vel) const;
	
	
private:
	void SetUpGraphics();
	void MakeStars(int stars, int width);
	
	
private:
	class Tile {
	public:
		Tile(std::vector<float>::iterator it);
		
		void Add(short x, short y);
		
		std::vector<float>::iterator first;
		std::vector<float>::iterator last;
	};
	
	
private:
	int widthMod;
	int tileMod;
	std::vector<float> data;
	std::vector<std::vector<Tile>> tiles;
	
	Shader shader;
	GLuint vao;
	GLuint vbo;
};



#endif
