/* renderBuffer.vert
Copyright (c) 2023 by thewierdnut

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

uniform vec2 size;
uniform vec2 position;
uniform vec2 scale;
uniform vec2 srcposition;
uniform vec2 srcscale;

in vec2 vert;
out vec2 tpos;
out vec2 vpos;

void main()
{
	gl_Position = vec4((position + vert * size) * scale, 0, 1);
	// Convert from vertex to texture coordinates.
	vpos = vert + vec2(.5, .5);
	vec2 tsize = size * srcscale;
	vec2 tsrc = srcposition * srcscale;
	tpos = vpos * tsize + tsrc;

	// Negative is up.
	tpos.y = 1.0 - tpos.y;
}
