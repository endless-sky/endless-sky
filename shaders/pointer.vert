/* pointer.vert
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

uniform vec2 scale;
uniform vec2 center;
uniform vec2 angle;
uniform vec2 size;
uniform float offset;

in vec2 vert;
out vec2 coord;

void main() {
	coord = vert * size.x;
	vec2 base = center + angle * (offset - size.y * (vert.x + vert.y));
	vec2 wing = vec2(angle.y, -angle.x) * (size.x * .5 * (vert.x - vert.y));
	gl_Position = vec4((base + wing) * scale, 0, 1);
}
