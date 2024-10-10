precision mediump float;

uniform vec2 scale;
uniform vec2 position;
uniform mat2 transform;
uniform vec2 blur;
uniform float clip;

in vec2 vert;
out vec2 fragTexCoord;

void main() {
	vec2 blurOff = 2.f * vec2(vert.x * abs(blur.x), vert.y * abs(blur.y));
	gl_Position = vec4((transform * (vert + blurOff) + position) * scale, 0, 1);
	vec2 texCoord = vert + vec2(.5, .5);
	fragTexCoord = vec2(texCoord.x, min(clip, texCoord.y)) + blurOff;
}