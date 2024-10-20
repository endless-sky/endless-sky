precision mediump float;

uniform vec2 scale;
uniform vec2 center;
uniform vec2 angle;
uniform vec2 size;
uniform float offset;

in vec2 vert;
out vec2 coord;

void main() {
	coord = vert * size.x;
	vec2 base = center + angle * (offset - size.y * (vert.x + vert.y));
	vec2 wing = vec2(angle.y, -angle.x) * (size.x * .5 * (vert.x - vert.y));
	gl_Position = vec4((base + wing) * scale, 0, 1);
}
