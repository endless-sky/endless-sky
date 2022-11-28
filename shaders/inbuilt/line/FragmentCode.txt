// fragment line shader
precision mediump float;
uniform vec4 color;

in vec2 tpos;
in float tscale;
out vec4 finalColor;

void main()
{
	float alpha = min(tscale - abs(tpos.x * (2.f * tscale) - tscale), 1.f - abs(tpos.y));
	finalColor = color * alpha;
}
