/* line.vert
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

uniform vec2 scale;
uniform highp vec2 start;
uniform highp vec2 end;
uniform float width;
// Explicitly using mediump here to account for buggy gles implementations.
uniform mediump int cap;

uniform vec4 startColor;
uniform vec4 endColor;

in vec2 vert;
out vec2 pos;
out vec4 color;

void main() {
	// Construct a rectangle around the line that can accommodate a line of width "width".
	vec2 unit = normalize(end - start);
	// The vertex will originate from the start or endpoint of the line, depending on the input vertex data.
	highp vec2 origin = vert.y > 0.0 ? start : end;
	color = vert.y > 0.0 ? startColor : endColor;
	// Pad the width by 1 so the SDFs have enough space to naturally anti-alias.
	float widthOffset = width + 1.;
	// If the cap is rounded, offset along the unit vector by the width, as the cap is circular with radius
	// "width" from the start/endpoints. This is also padded by 1 to allow for anti-aliasing.
	float capOffset = (cap == 1) ? widthOffset : 1.;
	// The vertex position is the originating position plus an offset away from the line.
	// The offset is a combination of a perpendicular offset of widthOffset and a normal offset of capOffset
	// that is flipped into a different direction for each vertex, resulting in a rectangle that tightly
	// covers the bounds of the line.
	pos = origin + vec2(unit.y, -unit.x) * vert.x * widthOffset - unit * capOffset * vert.y;
	// Transform the vertex position into es coordinates, so it can easily be consumed by the fragment shader,
	// which has access to the start/end points of the line in es' coordinate system.
	gl_Position = vec4(pos / scale, 0, 1);
	gl_Position.y = -gl_Position.y;
	gl_Position.xy *= 2.0;
}
