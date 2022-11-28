// fragment fog shader
#ifdef ES_GLES
precision mediump sampler2D;
#endif
precision mediump float;
uniform sampler2D tex;

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
	finalColor = vec4(0, 0, 0, texture(tex, fragTexCoord).r);
}
