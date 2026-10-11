/* batch.vert
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

uniform vec2 scale;

in vec2 vert;
in vec3 texCoord;
in float alpha;
out vec3 fragTexCoord;
out float fragAlpha;

void main() {
	gl_Position = vec4(vert * scale, 0, 1);
	fragTexCoord = texCoord;
	fragAlpha = alpha;
}
