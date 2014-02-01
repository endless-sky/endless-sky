/* Font.h
Michael Zahniser, 16 Oct 2013

Class for drawing text in OpenGL.
*/

#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

#include "Point.h"
#include "Shader.h"

#include <GL/glew.h>

#include <string>



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
