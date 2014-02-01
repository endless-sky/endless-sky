/* StarField.h
Michael Zahniser, 10 Oct 2013.

Object to hold a set of "stars" to be drawn as a backdrop. Each star has an
(x, y) location and a size. For efficiency when handling such a large set of
points, the field is divided up into 256 x 256 grid squares, and you can query
for a list of all the stars in a given square.
*/

#ifndef STAR_FIELD_H_INCLUDED
#define STAR_FIELD_H_INCLUDED

#include "Point.h"
#include "Shader.h"

#include <GL/glew.h>

#include <vector>



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
