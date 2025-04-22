/* pointer.frag
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

uniform vec4 color;
uniform vec2 size;

in vec2 coord;
out vec4 finalColor;

void main() {
	float height = (coord.x + coord.y) / size.x;
	float taper = height * height * height;
	taper *= taper * .5 * size.x;
	float alpha = clamp(.8 * min(coord.x, coord.y) - taper, 0.f, 1.f);
	alpha *= clamp(1.8 * (1. - height), 0.f, 1.f);
	finalColor = color * alpha;
}
