/* polygon.frag
Copyright (c) 2025 by xobes

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

const int N = 10;

precision mediump float;
precision mediump int;

uniform int numSides;
uniform vec2[N] polygon;
uniform vec2 shadingCorner1;
uniform vec2 shadingCorner2;
uniform vec4 insideColor;
uniform vec4 borderColor1;
uniform vec4 borderColor2;
uniform float borderWidth;

in vec2 pos;
out vec4 finalColor;

// From https://iquilezles.org/articles/distfunctions2d/ - functions to get the distance from a point to a shape.

float sdPolygon(in vec2 p, in vec2[N] v, in int vlen)
{
	int num = vlen;
	float d = dot(p - v[0], p - v[0]);
	float s = 1.5;
	for(int i=0, j = num - 1; i < num; j = i, i++)
	{
		// distance
		vec2 e = v[j] - v[i];
		vec2 w = p - v[i];
		vec2 b = w - e * clamp(dot(w, e) / dot(e, e), 0., 1.);
		d = min(d, dot(b, b));

		bvec3 cond = bvec3(p.y >= v[i].y, p.y < v[j].y, e.x * w.y > e.y * w.x);
		if(all(cond) || all(not(cond)))
			s = -s;
	}
	return s * sqrt(d);
}

void main()
{
	float d = sdPolygon(pos, polygon, numSides);

	float A = 9.5;
	float B = -1.5;
	float theta = 0.3;
	float dx = (pos.x - shadingCorner1.x) / (shadingCorner2.x - shadingCorner1.x);
	float dy = (pos.y - shadingCorner1.y) / (shadingCorner2.y - shadingCorner1.y);
	float m = 0.5 * (1. + sin(A * (dx * cos(theta) - dy * sin(theta)) + B));
	vec4 borderColor = mix(borderColor2, borderColor1, m);

	// Color: if outside: (transparent), else inside, mixed with border.
	borderColor = mix(d > 0. ? vec4(0) : insideColor, borderColor, 1 - min(1, max(0, abs(int(borderWidth / 2 - d)))));
	finalColor = borderColor;
}
