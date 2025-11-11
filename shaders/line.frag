/* line.frag
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

precision mediump float;
precision mediump int;

uniform highp vec2 start;
uniform highp vec2 end;
uniform float width;
// Explicitly using mediump here to account for buggy gles implementations.
uniform mediump int cap;

in vec2 pos;
in vec4 color;
out vec4 finalColor;

// From https://iquilezles.org/articles/distfunctions2d/ - functions to get the distance from a point to a shape.

float sdSegment(highp vec2 p, highp vec2 a, highp vec2 b) {
	highp vec2 ab = b - a;
	highp vec2 ap = p - a;
	float h = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
	return length(ap - h * ab);
}

float sdOrientedBox(highp vec2 p, highp vec2 a, highp vec2 b, highp float th) {
	float l = length(b - a);
	vec2 d = (b - a) / l;
	vec2 q = (p - (a + b) * 0.5);
	q = mat2(d.x, -d.y, d.y, d.x) * q;
	q = abs(q) - vec2(l, th) * 0.5;
	return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);
}

void main() {
	float dist;
	if (cap == 1) {
		// Rounded caps can shortcut to a segment sdf.
		// Segment sdf only provides a distance from the line itself so we manually subtract it from the width.
		dist = width - sdSegment(pos, start, end);
	} else {
		// Subtract from 1 here to add some AA.
		dist = 1. - sdOrientedBox(pos, start, end, width);
	}
	float alpha = clamp(dist, 0.0, 1.0);
	finalColor = color * alpha;
}
