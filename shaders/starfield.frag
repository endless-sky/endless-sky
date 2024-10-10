precision mediump float;

in float fragmentAlpha;
in vec2 coord;
out vec4 finalColor;

void main() {
	float alpha = fragmentAlpha * (1. - abs(coord.x) - abs(coord.y));
	finalColor = vec4(1, 1, 1, 1) * alpha;
}