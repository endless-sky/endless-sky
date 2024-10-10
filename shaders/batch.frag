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