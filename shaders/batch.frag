/* batch.frag
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

precision mediump float;
precision mediump sampler2DArray;

uniform sampler2DArray tex;
uniform float frameCount;

in vec3 fragTexCoord;
in float fragAlpha;
out vec4 finalColor;

void main() {
	float first = floor(fragTexCoord.z);
	float second = mod(ceil(fragTexCoord.z), frameCount);
	float fade = fragTexCoord.z - first;
	finalColor = mix(
		texture(tex, vec3(fragTexCoord.xy, first)),
		texture(tex, vec3(fragTexCoord.xy, second)), fade);
	finalColor *= vec4(fragAlpha);
}
