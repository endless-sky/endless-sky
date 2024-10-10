uniform vec2 scale;

in vec2 vert;
in vec3 texCoord;
in float alpha;
out vec3 fragTexCoord;
out float fragAlpha;

void main() {
	gl_Position = vec4(vert * scale, 0, 1);
	fragTexCoord = texCoord;
	fragAlpha = alpha;
}