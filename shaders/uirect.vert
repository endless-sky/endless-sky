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
uniform vec2 scale;
uniform vec2 center;
uniform vec2 size;

in vec2 vert;
out float borderGradient;
out vec2 screenCoords;

void main() {
	screenCoords = center + vert * size;
	gl_Position = vec4(screenCoords * scale, 0, 1);
	borderGradient = vert.x - vert.y;
}
