/* ring.frag
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
uniform float radius;
uniform float width;
uniform float angle;
uniform float startAngle;
uniform float dash;
const float pi = 3.1415926535897932384626433832795;

in vec2 coord;
out vec4 finalColor;

void main() {
	float arc = mod(atan(coord.x, coord.y) + pi + startAngle, 2.f * pi);
	float arcFalloff = 1.f - min(2.f * pi - arc, arc - angle) * radius;
	if(dash != 0.f)
	{
		arc = mod(arc, dash);
		arcFalloff = min(arcFalloff, min(arc, dash - arc) * radius);
	}
	float len = length(coord);
	float lenFalloff = width - abs(len - radius);
	float alpha = clamp(min(arcFalloff, lenFalloff), 0.f, 1.f);
	finalColor = color * alpha;
}
