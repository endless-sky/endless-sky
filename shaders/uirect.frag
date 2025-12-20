/* starfield.vert
Copyright (c) 2025 by thewierdnut

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
uniform vec4 bg1;
uniform vec4 bg2;
uniform vec4 bg3;
uniform vec2 center;
uniform vec2 size;

out vec4 finalColor;
in float borderGradient;
in vec2 screenCoords;


void main() {
	if (screenCoords.x - 1.0 > center.x - size.x / 2.0 &&
		screenCoords.y - 1.0 > center.y - size.y / 2.0 &&
		screenCoords.y + 1.0 < center.y + size.y / 2.0 &&
		screenCoords.x + 1.0 < center.x + size.x / 2.0)
		finalColor = color;
	else
	{
		if (borderGradient < 0.0)
			finalColor = mix(bg2, bg1, -borderGradient);
		else
			finalColor = mix(bg2, bg3, borderGradient);
	}
}