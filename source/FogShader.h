/* FogShader.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FOG_SHADER_H_
#define FOG_SHADER_H_

class PlayerInfo;
class Point;



// Shader for drawing a "fog of war" overlay on the map.
class FogShader {
public:
	static void Init();
	static void Redraw();
	static void Draw(const Point &center, double zoom, const PlayerInfo &player);
};



#endif
