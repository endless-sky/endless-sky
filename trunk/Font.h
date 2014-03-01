/* Font.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FONT_H_
#define FONT_H_

#include "Point.h"
#include "Shader.h"

#include <GL/glew.h>

#include <string>



// Class for drawing text in OpenGL.
class Font {
public:
	Font();
	Font(const std::string &imagePath);
	
	void Load(const std::string &imagePath);
	
	void Draw(const std::string &str, int x, int y) const;
	void Draw(const char *str, int x, int y) const;
	
	void Draw(const std::string &str, const Point &point, const float *color = nullptr) const;
	void Draw(const char *str, const Point &point, const float *color = nullptr) const;
	
	int Width(const std::string &str, char after = ' ') const;
	int Width(const char *str, char after = ' ') const;
	
	int Height() const;
	
	int Space() const;
	
	
private:
	void LoadTexture(class SDL_Surface *bmp);
	void CalculateAdvances(class SDL_Surface *bmp);
	void SetUpShader(float glyphW, float glyphH);
	
	
private:
	Shader shader;
	GLuint texture;
	GLuint vao;
	GLuint vbo;
	
	int height;
	int space;
	mutable int screenWidth;
	mutable int screenHeight;
	
	static const int GLYPHS = 96;
	int advance[GLYPHS * GLYPHS];
};



#endif
