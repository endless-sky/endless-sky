/* outline.frag
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

// The outline shader applies a Sobel filter to an image. The alpha channel
// (i.e. the silhouette of the sprite) is given the most weight, but some
// weight is also given to the RGB values so that there will be some detail
// in the interior of the silhouette as well.

// To reduce sampling error and bring out fine details, for every output
// pixel the Sobel filter is actually applied in a 3x3 neighborhood and
// averaged together. That neighborhood's scale is .618034 times the scale
// of the Sobel neighborhood (i.e. the golden ratio) to minimize any
// aliasing effects between the two.

precision mediump float;
precision mediump sampler2DArray;

uniform sampler2DArray tex;
uniform float frame;
uniform float frameCount;
uniform vec4 color;
uniform vec2 off;
const vec4 weight = vec4(.4, .4, .4, 1.);

in vec2 fragTexCoord;
out vec4 finalColor;

float Sobel(float layer) {
	float sum = 0.f;
	for(int dy = -1; dy <= 1; ++dy)
	{
		for(int dx = -1; dx <= 1; ++dx)
		{
			vec2 center = fragTexCoord + .618034 * off * vec2(dx, dy);
			float nw = dot(texture(tex, vec3(center + vec2(-off.x, -off.y), layer)), weight);
			float ne = dot(texture(tex, vec3(center + vec2(off.x, -off.y), layer)), weight);
			float sw = dot(texture(tex, vec3(center + vec2(-off.x, off.y), layer)), weight);
			float se = dot(texture(tex, vec3(center + vec2(off.x, off.y), layer)), weight);
			float h = nw + sw - ne - se + 2.f * (
				dot(texture(tex, vec3(center + vec2(-off.x, 0.f), layer)), weight)
				- dot(texture(tex, vec3(center + vec2(off.x, 0.f), layer)), weight));
			float v = nw + ne - sw - se + 2.f * (
				dot(texture(tex, vec3(center + vec2(0.f, -off.y), layer)), weight)
				- dot(texture(tex, vec3(center + vec2(0.f, off.y), layer)), weight));
			sum += h * h + v * v;
		}
	}
	return sum;
}

void main() {
	float first = floor(frame);
	float second = mod(ceil(frame), frameCount);
	float fade = frame - first;
	float sum = mix(Sobel(first), Sobel(second), fade);
	finalColor = color * sqrt(sum / 180.f);
}
