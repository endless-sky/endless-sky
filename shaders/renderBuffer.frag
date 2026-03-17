/* renderBuffer.frag
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
precision mediump sampler2D;

uniform sampler2D tex;
uniform vec4 fade;

in vec2 tpos;
in vec2 vpos;
out vec4 finalColor;

void main() {
	float epsilon = .001;
	float weightTop = clamp((vpos.y + epsilon) / (fade[0] + epsilon), 0.0, 1.0);
	float weightBottom = clamp(((1.0 - vpos.y) + epsilon) / (fade[1] + epsilon), 0.0, 1.0);
	float weightLeft = clamp((vpos.x + epsilon) / (fade[2] + epsilon), 0.0, 1.0);
	float weightRight = clamp(((1.0 - vpos.x) + epsilon) / (fade[3] + epsilon), 0.0, 1.0);
	float weight = min(min(min(weightTop, weightBottom), weightLeft), weightRight);
	if(tpos.x > 0.0 && tpos.y > 0.0 &&
			tpos.x < 1.0 && tpos.y < 1.0)
		finalColor = texture(tex, tpos) * weight;
	else
		discard;
}
