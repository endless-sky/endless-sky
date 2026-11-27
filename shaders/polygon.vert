/* polygon.vert
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

const int N = 7;

precision mediump float;
precision mediump int;

uniform vec2 scale;

in vec2 vert;
out vec2 pos;

void main() {
	pos = vert / scale;
	gl_Position = vec4(vert, 0, 1);
}
