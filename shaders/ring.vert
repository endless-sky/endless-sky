precision mediump float;

uniform vec2 scale;
uniform vec2 position;
uniform float radius;
uniform float width;

in vec2 vert;
out vec2 coord;

void main() {
	coord = (radius + width) * vert;
	gl_Position = vec4((coord + position) * scale, 0.f, 1.f);
}