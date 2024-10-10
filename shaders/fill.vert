uniform vec2 scale;
uniform vec2 center;
uniform vec2 size;

in vec2 vert;

void main() {
	gl_Position = vec4((center + vert * size) * scale, 0, 1);
}
