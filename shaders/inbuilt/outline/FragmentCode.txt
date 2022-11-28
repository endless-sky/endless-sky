// fragment outline shader
precision mediump float;
#ifdef ES_GLES
precision mediump sampler2DArray;
#endif
uniform sampler2DArray tex;
uniform float frame;
uniform float frameCount;
uniform vec4 color;
uniform vec2 off;
const vec4 weight = vec4(.4, .4, .4, 1.);

in vec2 fragTexCoord;

out vec4 finalColor;

float Sobel(float layer)
{
	float sum = 0.f;
	for(int dy = -1; dy <= 1; ++dy)
	{
		for(int dx = -1; dx <= 1; ++dx)
		{
			vec2 center = fragTexCoord + .618134 * off * vec2(dx, dy);
			float nw = dot(texture(tex, vec3(center + vec2(-off.x, -off.y), layer)), weight);
			float ne = dot(texture(tex, vec3(center + vec2(off.x, -off.y), layer)), weight);
			float sw = dot(texture(tex, vec3(center + vec2(-off.x, off.y), layer)), weight);
			float se = dot(texture(tex, vec3(center + vec2(off.x, off.y), layer)), weight);
			float h = nw + sw - ne - se + 2.f * (
				dot(texture(tex, vec3(center + vec2(-off.x, 0.f), layer)), weight)
				- dot(texture(tex, vec3(center + vec2(off.x, 0.f), layer)), weight));
			float v = nw + ne - sw - se + 2.f * (
				dot(texture(tex, vec3(center + vec2(0.f, -off.y), layer)), weight)
				- dot(texture(tex, vec3(center + vec2(0.f, off.y), layer)), weight));
			sum += h * h + v * v;
		}
	}
	return sum;
}

void main()
{
	float first = floor(frame);
	float second = mod(ceil(frame), frameCount);
	float fade = frame - first;
	float sum = mix(Sobel(first), Sobel(second), fade);
	finalColor = color * sqrt(sum / 180.f);
}
