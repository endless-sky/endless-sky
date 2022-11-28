// vertex line shader
uniform vec2 scale;
uniform vec2 start;
uniform vec2 len;
uniform vec2 width;

in vec2 vert;
out vec2 tpos;
out float tscale;

void main()
{
	tpos = vert;
	tscale = length(len);
	gl_Position = vec4((start + vert.x * len + vert.y * width) * scale, 0, 1);
}
