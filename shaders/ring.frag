precision mediump float;

uniform vec4 color;
uniform float radius;
uniform float width;
uniform float angle;
uniform float startAngle;
uniform float dash;
const float pi = 3.1415926535897932384626433832795;

in vec2 coord;
out vec4 finalColor;

void main() {
	float arc = mod(atan(coord.x, coord.y) + pi + startAngle, 2.f * pi);
	float arcFalloff = 1.f - min(2.f * pi - arc, arc - angle) * radius;
	if(dash != 0.f)
	{
		arc = mod(arc, dash);
		arcFalloff = min(arcFalloff, min(arc, dash - arc) * radius);
	}
	float len = length(coord);
	float lenFalloff = width - abs(len - radius);
	float alpha = clamp(min(arcFalloff, lenFalloff), 0.f, 1.f);
	finalColor = color * alpha;
}
