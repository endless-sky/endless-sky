precision mediump float;

uniform vec4 color;
uniform vec2 size;

in vec2 coord;
out vec4 finalColor;

void main() {
	float height = (coord.x + coord.y) / size.x;
	float taper = height * height * height;
	taper *= taper * .5 * size.x;
	float alpha = clamp(.8 * min(coord.x, coord.y) - taper, 0.f, 1.f);
	alpha *= clamp(1.8 * (1. - height), 0.f, 1.f);
	finalColor = color * alpha;
}