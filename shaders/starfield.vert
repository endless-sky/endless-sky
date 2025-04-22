/* starfield.vert
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

uniform mat2 rotate;
uniform vec2 translate;
uniform vec2 scale;
uniform float elongation;
uniform float brightness;

in vec2 offset;
in float size;
in float corner;
out float fragmentAlpha;
out vec2 coord;

void main() {
	fragmentAlpha = brightness * (4. / (4. + elongation)) * size * .2 + .05;
	coord = vec2(sin(corner), cos(corner));
	vec2 elongated = vec2(coord.x * size, coord.y * (size + elongation));
	gl_Position = vec4((rotate * elongated + translate + offset) * scale, 0, 1);
}
