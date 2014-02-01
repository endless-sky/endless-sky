/* StarField.cpp
Michael Zahniser, 10 Oct 2013

Function definitions for the StarField class.
*/

#include "StarField.h"

#include "Screen.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>

using namespace std;



void StarField::Init(int stars, int width)
{
	SetUpGraphics();
	MakeStars(stars, width);
}



void StarField::Draw(Point pos, Point vel) const
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	float length = vel.Length();
	Point unit = length ? vel.Unit() : Point(1., 0.);
	
	double screenWidth = Screen::Width();
	double screenHeight = Screen::Height();
	double widthScale = 2. / screenWidth;
	double heightScale = 2. / screenHeight;
	
	Point uw = unit * widthScale;
	Point uh = unit * heightScale;
	
	GLfloat rotate[4] = {
		static_cast<float>(-uw.Y()), static_cast<float>(-uh.X()),
		static_cast<float>(-uw.X()), static_cast<float>(uh.Y())};
	glUniformMatrix2fv(shader.Uniform("rotate"), 1, false, rotate);
	
	float alpha = 4.f / (4.f + length);
	glUniform1f(shader.Uniform("alpha"), alpha);
	
	long minX = pos.X() - screenWidth / 2 - static_cast<long>(fabs(vel.X()) + 1);
	long minY = pos.Y() - screenHeight / 2 - static_cast<long>(fabs(vel.Y()) + 1);
	long maxX = minX + screenWidth + 2 * static_cast<long>(fabs(vel.X()) + 1);
	long maxY = minY + screenHeight + 2 * static_cast<long>(fabs(vel.Y()) + 1);
	minX &= ~255l;
	minY &= ~255l;
	
	for(long gy = minY; gy < maxY; gy += 256)
		for(long gx = minX; gx < maxX; gx += 256)
		{
			Point off = pos - Point(gx, gy);
			
			const Tile &tile = tiles[(gx & widthMod) / 256][(gy & widthMod) / 256];
			auto it = tile.first;
			auto end = tile.last;
			
			for(auto it = tile.first; it != tile.last; it += 3)
			{
				GLfloat translate[2] = {
					static_cast<float>((it[0] - off.X()) * widthScale),
					static_cast<float>((it[1] - off.Y()) * -heightScale)};
				glUniform2fv(shader.Uniform("translate"), 1, translate);
				
				GLfloat scale[2] = {it[2], it[2] + length};
				glUniform2fv(shader.Uniform("scale"), 1, scale);
				
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
			}
		}
	
	glBindVertexArray(0);
	glUseProgram(0);
}



void StarField::SetUpGraphics()
{
	static const char *vertexCode =
		"#version 130\n"
		"uniform mat2 rotate;\n"
		"uniform float alpha;\n"
		"uniform vec2 scale;\n"
		"uniform vec2 translate;\n"
		
		"in vec2 vert;\n"
		"in float vertAlpha;\n"
		"out float fragmentAlpha;\n"
		
		"void main() {\n"
		"  fragmentAlpha = alpha * vertAlpha * scale.x * .1 + .05;\n"
		"  gl_Position = vec4(rotate * (vert * scale) + translate, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"#version 130\n"
		
		"in float fragmentAlpha;\n"
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  finalColor = vec4(1, 1, 1, 1) * fragmentAlpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	
	// make and bind the VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	// make and bind the VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// Stars are a diamond that is opaque in the center and transparent on the
	// edges. Store this as a triangle strip (4 non-degenerate triangles).
	GLfloat vertexData[] = {
		-.5f,  0.f,  0.f,
		 0.f, -.5f,  0.f,
		 0.f,  0.f,  1.f,
		 .5f,  0.f,  0.f,
		 .5f,  0.f,  0.f,
		 0.f,  .5f,  0.f,
		 0.f,  0.f,  1.f,
		-.5f,  0.f,  0.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	// connect the xy to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE,
		3 * sizeof(GLfloat), NULL);
	
	glEnableVertexAttribArray(shader.Attrib("vertAlpha"));
	glVertexAttribPointer(shader.Attrib("vertAlpha"), 1, GL_FLOAT, GL_FALSE,
		3 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void StarField::MakeStars(int stars, int width)
{
	// We can only work with power-of-two widths above 256.
	assert(width >= 256 && !(width & (width - 1)));
	
	data.resize(stars * 3);
	
	tiles.resize(width / 256);
	for(auto &row : tiles)
		row.resize(tiles.size(), Tile(data.begin()));
	
	widthMod = width - 1;
	tileMod = tiles.size() - 1;
	
	vector<short> off;
	static const short MAX_OFF = 50;
	static const short MAX_D = MAX_OFF * MAX_OFF;
	static const short MIN_D = MAX_D / 4;
	off.reserve(MAX_OFF * MAX_OFF * 5);
	for(short x = -MAX_OFF; x <= MAX_OFF; ++x)
		for(short y = -MAX_OFF; y <= MAX_OFF; ++y)
		{
			short d = x * x + y * y;
			if(d < MIN_D || d > MAX_D)
				continue;
			
			off.push_back(x);
			off.push_back(y);
		}
	
	// Generate random points in a temporary vector.
	// Keep track of how many fall into each tile, for sorting out later.
	vector<short> temp;
	temp.reserve(2 * stars);
	
	short x = rand() % width;
	short y = rand() % width;
	for(int i = 0; i < stars; ++i)
	{
		for(int j = 0; j < 10; ++j)
		{
			int index = (rand() % off.size()) & ~1;
			x += off[index];
			y += off[index + 1];
			x &= widthMod;
			y &= widthMod;
		}
		temp.push_back(x);
		temp.push_back(y);
		tiles[y / 256][x / 256].last += 3;
	}
	
	// Figure out where in the array we will store data for each point.
	int total = 0;
	for(auto &row : tiles)
		for(auto &tile : row)
		{
			tile.first += total;
			total += tile.last - data.begin();
			tile.last = tile.first;
		}
	
	for(auto it = temp.begin(); it != temp.end(); )
	{
		short x = *it++;
		short y = *it++;
		tiles[y / 256][x / 256].Add(x, y);
	}
}



StarField::Tile::Tile(vector<float>::iterator it)
	: first(it), last(it)
{
}



void StarField::Tile::Add(short x, short y)
{
	short random = rand() % 4096;
	*last++ = (x & 255) + (random & 15) * 0.0625f;
	*last++ = (y & 255) + (random >> 8) * 0.0625f;
	*last++ = (((random >> 4) & 15) + 20) * 0.125f;
}
