/* StarField.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "StarField.h"

#include "Angle.h"
#include "Body.h"
#include "DrawList.h"
#include "pi.h"
#include "Point.h"
#include "Preferences.h"
#include "Random.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"

#include <cmath>
#include <numeric>

using namespace std;

namespace {
	static const int TILE_SIZE = 256;
	// The star field tiles in 4000 pixel increments. Have the tiling of the haze
	// field be as different from that as possible. (Note: this may need adjusting
	// in the future if monitors larger than this width ever become commonplace.)
	static const double HAZE_WRAP = 6627.;
	// Don't let two haze patches be closer to each other than this distance. This
	// avoids having very bright haze where several patches overlap.
	static const double HAZE_DISTANCE = 1200.;
	// This is how many haze fields should be drawn.
	static const size_t HAZE_COUNT = 16;
}



void StarField::Init(int stars, int width)
{
	SetUpGraphics();
	MakeStars(stars, width);
	
	const Sprite *sprite = SpriteSet::Get("_menu/haze");
	for(size_t i = 0; i < HAZE_COUNT; ++i)
	{
		Point next;
		bool overlaps = true;
		while(overlaps)
		{
			next = Point(Random::Real() * HAZE_WRAP, Random::Real() * HAZE_WRAP);
			overlaps = false;
			for(const Body &other : haze)
			{
				Point previous = other.Position();
				double dx = remainder(previous.X() - next.X(), HAZE_WRAP);
				double dy = remainder(previous.Y() - next.Y(), HAZE_WRAP);
				if(dx * dx + dy * dy < HAZE_DISTANCE * HAZE_DISTANCE)
				{
					overlaps = true;
					break;
				}
			}
		}
		haze.emplace_back(sprite, next, Point(), Angle::Random(), 8.);
	}
}



void StarField::SetHaze(const Sprite *sprite)
{
	// If no sprite is given, set the default one.
	if(!sprite)
		sprite = SpriteSet::Get("_menu/haze");
	
	for(Body &body : haze)
		body.SetSprite(sprite);
}



void StarField::Draw(const Point &pos, const Point &vel, double zoom) const
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	float length = vel.Length();
	Point unit = length ? vel.Unit() : Point(1., 0.);
	// Don't zoom the stars at the same rate as the field; otherwise, at the
	// farthest out zoom they are too small to draw well.
	unit /= sqrt(zoom);
	
	float baseZoom = static_cast<float>(2. * zoom);
	GLfloat scale[2] = {baseZoom / Screen::Width(), -baseZoom / Screen::Height()};
	glUniform2fv(scaleI, 1, scale);
	
	GLfloat rotate[4] = {
		static_cast<float>(unit.Y()), static_cast<float>(-unit.X()),
		static_cast<float>(unit.X()), static_cast<float>(unit.Y())};
	glUniformMatrix2fv(rotateI, 1, false, rotate);
	
	glUniform1f(lengthI, length);
	
	// Stars this far beyond the border may still overlap the screen.
	double borderX = fabs(vel.X()) + 1.;
	double borderY = fabs(vel.Y()) + 1.;
	// Find the absolute bounds of the star field we must draw.
	int minX = pos.X() + (Screen::Left() - borderX) / zoom;
	int minY = pos.Y() + (Screen::Top() - borderY) / zoom;
	int maxX = pos.X() + (Screen::Right() + borderX) / zoom;
	int maxY = pos.Y() + (Screen::Bottom() + borderY) / zoom;
	// Round down to the start of the nearest tile.
	minX &= ~(TILE_SIZE - 1l);
	minY &= ~(TILE_SIZE - 1l);
	
	for(int gy = minY; gy < maxY; gy += TILE_SIZE)
		for(int gx = minX; gx < maxX; gx += TILE_SIZE)
		{
			Point off = Point(gx, gy) - pos;
			GLfloat translate[2] = {
				static_cast<float>(off.X()),
				static_cast<float>(off.Y())
			};
			glUniform2fv(translateI, 1, translate);
			
			int index = (gx & widthMod) / TILE_SIZE + ((gy & widthMod) / TILE_SIZE) * tileCols;
			int first = 6 * tileIndex[index];
			int count = 6 * tileIndex[index + 1] - first;
			glDrawArrays(GL_TRIANGLES, first, count);
		}
	
	glBindVertexArray(0);
	glUseProgram(0);
	
	// Draw the background haze unless it is disabled in the preferences.
	if(!Preferences::Has("Draw background haze"))
		return;
	
	DrawList drawList;
	drawList.Clear(0, zoom);
	drawList.SetCenter(pos);
	
	// Any object within this range must be drawn. Some haze sprites may repeat
	// more than once if the view covers a very large area.
	Point size = Point(1., 1.) * haze.front().Radius();
	Point topLeft = pos + (Screen::TopLeft() - size) / zoom;
	Point bottomRight = pos + (Screen::BottomRight() + size) / zoom;
	for(const Body &it : haze)
	{
		// Figure out the position of the first instance of this haze that is to
		// the right of and below the top left corner of the screen.
		double startX = fmod(it.Position().X() - topLeft.X(), HAZE_WRAP);
		startX += topLeft.X() + HAZE_WRAP * (startX < 0.);
		double startY = fmod(it.Position().Y() - topLeft.Y(), HAZE_WRAP);
		startY += topLeft.Y() + HAZE_WRAP * (startY < 0.);
	
		// Draw any instances of this haze that are on screen.
		for(double y = startY; y < bottomRight.Y(); y += HAZE_WRAP)
			for(double x = startX; x < bottomRight.X(); x += HAZE_WRAP)
				drawList.Add(it, Point(x, y));
	}
	drawList.Draw();
}



void StarField::SetUpGraphics()
{
	static const char *vertexCode =
		"uniform mat2 rotate;\n"
		"uniform vec2 translate;\n"
		"uniform vec2 scale;\n"
		"uniform float elongation;\n"
		
		"attribute vec2 offset;\n"
		"attribute float size;\n"
		"attribute float corner;\n"
		"varying float fragmentAlpha;\n"
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  fragmentAlpha = (4. / (4. + elongation)) * size * .2 + .05;\n"
		"  coord = vec2(sin(corner), cos(corner));\n"
		"  vec2 elongated = vec2(coord.x * size, coord.y * (size + elongation));\n"
		"  gl_Position = vec4((rotate * elongated + translate + offset) * scale, 0, 1);\n"
		"}\n";

	static const char *fragmentCode =
		"varying float fragmentAlpha;\n"
		"varying vec2 coord;\n"
		
		"void main() {\n"
		"  float alpha = fragmentAlpha * (1. - abs(coord.x) - abs(coord.y));\n"
		"  gl_FragColor = vec4(1, 1, 1, 1) * alpha;\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	
	// make and bind the VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	// make and bind the VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	offsetI = shader.Attrib("offset");
	sizeI = shader.Attrib("size");
	cornerI = shader.Attrib("corner");
	
	scaleI = shader.Uniform("scale");
	rotateI = shader.Uniform("rotate");
	lengthI = shader.Uniform("elongation");
	translateI = shader.Uniform("translate");
}



void StarField::MakeStars(int stars, int width)
{
	// We can only work with power-of-two widths above 256.
	if(width < TILE_SIZE || (width & (width - 1)))
		return;
	
	widthMod = width - 1;
	
	tileCols = (width / TILE_SIZE);
	tileIndex.clear();
	tileIndex.resize(tileCols * tileCols, 0);
	
	vector<int> off;
	static const int MAX_OFF = 50;
	static const int MAX_D = MAX_OFF * MAX_OFF;
	static const int MIN_D = MAX_D / 4;
	off.reserve(MAX_OFF * MAX_OFF * 5);
	for(int x = -MAX_OFF; x <= MAX_OFF; ++x)
		for(int y = -MAX_OFF; y <= MAX_OFF; ++y)
		{
			int d = x * x + y * y;
			if(d < MIN_D || d > MAX_D)
				continue;
			
			off.push_back(x);
			off.push_back(y);
		}
	
	// Generate random points in a temporary vector.
	// Keep track of how many fall into each tile, for sorting out later.
	vector<int> temp;
	temp.reserve(2 * stars);
	
	int x = Random::Int(width);
	int y = Random::Int(width);
	for(int i = 0; i < stars; ++i)
	{
		for(int j = 0; j < 10; ++j)
		{
			int index = Random::Int(off.size()) & ~1;
			x += off[index];
			y += off[index + 1];
			x &= widthMod;
			y &= widthMod;
		}
		temp.push_back(x);
		temp.push_back(y);
		int index = (x / TILE_SIZE) + (y / TILE_SIZE) * tileCols;
		++tileIndex[index];
	}
	
	// Accumulate item counts so that tileIndex[i] is the index in the array of
	// the first star that falls within tile i, and tileIndex.back() == stars.
	tileIndex.insert(tileIndex.begin(), 0);
	tileIndex.pop_back();
	partial_sum(tileIndex.begin(), tileIndex.end(), tileIndex.begin());
	
	// Each star consists of five vertices, each with four data elements.
	vector<GLfloat> data(6 * 4 * stars, 0.f);
	for(auto it = temp.begin(); it != temp.end(); )
	{
		// Figure out what tile this star is in.
		int x = *it++;
		int y = *it++;
		int index = (x / TILE_SIZE) + (y / TILE_SIZE) * tileCols;
		
		// Randomize its sub-pixel position and its size / brightness.
		int random = Random::Int(4096);
		float fx = (x & (TILE_SIZE - 1)) + (random & 15) * 0.0625f;
		float fy = (y & (TILE_SIZE - 1)) + (random >> 8) * 0.0625f;
		float size = (((random >> 4) & 15) + 20) * 0.0625f;
		
		// Fill in the data array.
		auto dataIt = data.begin() + 6 * 4 * tileIndex[index]++;
		const float CORNER[6] = {
			static_cast<float>(0. * PI),
			static_cast<float>(.5 * PI),
			static_cast<float>(1.5 * PI),
			static_cast<float>(.5 * PI),
			static_cast<float>(1.5 * PI),
			static_cast<float>(1. * PI)
		};
		for(float corner : CORNER)
		{
			*dataIt++ = fx;
			*dataIt++ = fy;
			*dataIt++ = size;
			*dataIt++ = corner;
		}
	}
	// Adjust the tile indices so that tileIndex[i] is the start of tile i.
	tileIndex.insert(tileIndex.begin(), 0);

	glBufferData(GL_ARRAY_BUFFER, sizeof(data.front()) * data.size(), data.data(), GL_STATIC_DRAW);
	
	// connect the xy to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(offsetI);
	glVertexAttribPointer(offsetI, 2, GL_FLOAT, GL_FALSE,
		4 * sizeof(GLfloat), nullptr);
	
	glEnableVertexAttribArray(sizeI);
	glVertexAttribPointer(sizeI, 1, GL_FLOAT, GL_FALSE,
		4 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));
	
	glEnableVertexAttribArray(cornerI);
	glVertexAttribPointer(cornerI, 1, GL_FLOAT, GL_FALSE,
		4 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
	
	// unbind the VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
