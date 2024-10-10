uniform mat2 rotate;
uniform vec2 translate;
uniform vec2 scale;
uniform float elongation;
uniform float brightness;

in vec2 offset;
in float size;
in float corner;
out float fragmentAlpha;
out vec2 coord;

void main() {
	fragmentAlpha = brightness * (4. / (4. + elongation)) * size * .2 + .05;
	coord = vec2(sin(corner), cos(corner));
	vec2 elongated = vec2(coord.x * size, coord.y * (size + elongation));
	gl_Position = vec4((rotate * elongated + translate + offset) * scale, 0, 1);
}